// Ponte JNI: saida de audio (Oboe/AAudio), entrada MIDI nativa (AMidi) e
// carregamento dos samples. As mensagens MIDI sao lidas DENTRO do callback de
// audio, sem troca de thread: a nota entra no proximo buffer a ser tocado.

#include <amidi/AMidi.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <oboe/Oboe.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "../engine/MidiParser.h"
#include "../engine/PianoEngine.h"

#define LOG_TAG "PianoMidi"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr int kMaxPorts = 8;

struct MidiSlot {
    std::atomic<AMidiOutputPort*> port{nullptr};
    AMidiDevice* device = nullptr;
    MidiParser parser;  // usado so pela thread de audio
};

class AudioOutput : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
public:
    explicit AudioOutput(PianoEngine& engine) : engine_(engine) {}

    bool open() {
        std::lock_guard<std::mutex> lock(streamMutex_);
        return openLocked();
    }

    void close() {
        std::lock_guard<std::mutex> lock(streamMutex_);
        wantRunning_ = false;
        if (stream_) {
            stream_->stop();
            stream_->close();
            stream_.reset();
        }
        tuner_.reset();
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) override {
        if (tuner_) tuner_->tune();
        pollMidi();
        auto* out = static_cast<float*>(data);
        if (stream->getChannelCount() == 2) {
            engine_.render(out, frames);
        } else {
            // Mono/multicanal (raro): renderiza em estereo e redistribui.
            const int ch = stream->getChannelCount();
            for (int off = 0; off < frames; off += kScratchFrames) {
                const int n = std::min(kScratchFrames, frames - off);
                engine_.render(scratch_, n);
                for (int i = 0; i < n; ++i)
                    for (int c = 0; c < ch; ++c)
                        out[(off + i) * ch + c] = c < 2 ? scratch_[2 * i + c] : 0.f;
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    void onErrorAfterClose(oboe::AudioStream*, oboe::Result error) override {
        // Fone conectado/desconectado, troca de rota etc.: reabre o stream.
        LOGI("stream fechado (%s), reabrindo", oboe::convertToText(error));
        std::lock_guard<std::mutex> lock(streamMutex_);
        stream_.reset();
        tuner_.reset();
        if (wantRunning_) openLocked();
    }

    // Retorna a porta MIDI para um slot livre; -1 se nao houver espaco.
    int attachPort(AMidiDevice* device, AMidiOutputPort* port) {
        for (int i = 0; i < kMaxPorts; ++i) {
            if (!slots_[i].port.load() && !slots_[i].device) {
                slots_[i].device = device;
                slots_[i].port.store(port, std::memory_order_release);
                return i;
            }
        }
        return -1;
    }

    // Desliga um slot garantindo que o callback nao esta lendo a porta.
    void detachPort(int slot, AMidiOutputPort** port, AMidiDevice** device) {
        *port = slots_[slot].port.exchange(nullptr);
        *device = slots_[slot].device;
        slots_[slot].device = nullptr;
        waitForCallbacks();
    }

    // Espera dois callbacks completos (ou o stream parado).
    void waitForCallbacks() {
        const uint64_t start = engine_.renderCount();
        for (int i = 0; i < 100 && engine_.renderCount() < start + 2; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    void fillStats(jint* s) {
        std::lock_guard<std::mutex> lock(streamMutex_);
        s[0] = stream_ ? stream_->getSampleRate() : 0;
        s[1] = stream_ ? stream_->getFramesPerBurst() : 0;
        s[2] = stream_ ? stream_->getBufferSizeInFrames() : 0;
        s[3] = stream_ ? stream_->getXRunCount().value() : 0;
        s[4] = stream_ && stream_->getSharingMode() == oboe::SharingMode::Exclusive;
        s[5] = stream_ && oboe::OboeExtensions::isMMapUsed(stream_.get());
        auto latency = stream_ ? stream_->calculateLatencyMillis() : oboe::ResultWithValue<double>(oboe::Result::ErrorNull);
        s[6] = latency ? static_cast<jint>(latency.value() * 10) : -1;  // decimos de ms
        s[7] = stream_ && stream_->getPerformanceMode() == oboe::PerformanceMode::LowLatency;
    }

private:
    static constexpr int kScratchFrames = 256;

    bool openLocked() {
        wantRunning_ = true;
        oboe::AudioStreamBuilder b;
        b.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::ChannelCount::Stereo)
            ->setUsage(oboe::Usage::Game)
            ->setContentType(oboe::ContentType::Music)
            ->setFormatConversionAllowed(true)
            ->setChannelConversionAllowed(true)
            ->setDataCallback(this)
            ->setErrorCallback(this);
        std::shared_ptr<oboe::AudioStream> stream;
        oboe::Result r = b.openStream(stream);
        if (r != oboe::Result::OK) {
            LOGE("falha ao abrir audio: %s", oboe::convertToText(r));
            return false;
        }
        engine_.prepare(static_cast<float>(stream->getSampleRate()));
        // Pede ao sistema para subir o clock da CPU quando a carga aumenta (acordes, pedal).
        stream->setPerformanceHintEnabled(true);
        // Comeca com 2 bursts; o LatencyTuner aumenta so se houver underruns.
        stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
        tuner_ = std::make_unique<oboe::LatencyTuner>(*stream);
        r = stream->requestStart();
        if (r != oboe::Result::OK) {
            LOGE("falha ao iniciar audio: %s", oboe::convertToText(r));
            stream->close();
            tuner_.reset();
            return false;
        }
        stream_ = stream;
        LOGI("audio: %d Hz, burst %d, buffer %d, mmap %d", stream->getSampleRate(),
             stream->getFramesPerBurst(), stream->getBufferSizeInFrames(),
             oboe::OboeExtensions::isMMapUsed(stream.get()));
        return true;
    }

    void pollMidi() {
        uint8_t buf[128];
        for (auto& slot : slots_) {
            AMidiOutputPort* port = slot.port.load(std::memory_order_acquire);
            if (!port) continue;
            for (int n = 0; n < 64; ++n) {
                int32_t opcode = 0;
                size_t bytes = 0;
                int64_t ts = 0;
                const ssize_t got = AMidiOutputPort_receive(port, &opcode, buf, sizeof(buf), &bytes, &ts);
                if (got <= 0) break;
                if (opcode == AMIDI_OPCODE_FLUSH) { slot.parser.reset(); continue; }
                slot.parser.feed(buf, bytes, [this](uint8_t s, uint8_t a, uint8_t b) { engine_.handleMidi(s, a, b); });
            }
        }
    }

    PianoEngine& engine_;
    std::mutex streamMutex_;
    std::shared_ptr<oboe::AudioStream> stream_;
    std::unique_ptr<oboe::LatencyTuner> tuner_;
    bool wantRunning_ = false;
    MidiSlot slots_[kMaxPorts];
    float scratch_[kScratchFrames * 2];
};

struct App {
    PianoEngine engine;
    AudioOutput audio{engine};
    jobject assetManagerRef = nullptr;
    AAssetManager* assets = nullptr;

    std::mutex bankMutex;
    std::unique_ptr<SampleBank> bank;
    std::thread loader;
    std::atomic<float> loadProgress{0.f};
    std::atomic<int> loadState{0};  // 0 ocioso, 1 carregando, 2 pronto, 3 erro
    std::atomic<bool> cancelLoad{false};
    std::atomic<long long> bankBytes{0};
};

App* gApp = nullptr;

bool readAsset(const std::string& name, std::vector<uint8_t>& out) {
    AAsset* a = AAssetManager_open(gApp->assets, name.c_str(), AASSET_MODE_BUFFER);
    if (!a) return false;
    const off_t len = AAsset_getLength(a);
    out.resize(static_cast<size_t>(len));
    const int read = AAsset_read(a, out.data(), out.size());
    AAsset_close(a);
    return read == len;
}

void unloadBank(App& app) {
    app.engine.setBank(nullptr);
    if (app.engine.bankInUse() != nullptr) app.audio.waitForCallbacks();
    std::lock_guard<std::mutex> lock(app.bankMutex);
    app.bank.reset();
    app.bankBytes = 0;
}

}  // namespace

#define JNI_FN(name) Java_com_gabrielnunes_pianomidi_NativeEngine_##name

extern "C" {

JNIEXPORT jboolean JNICALL JNI_FN(nativeCreate)(JNIEnv* env, jobject, jobject assetManager) {
    if (gApp) return JNI_TRUE;
    gApp = new App();
    gApp->assetManagerRef = env->NewGlobalRef(assetManager);
    gApp->assets = AAssetManager_fromJava(env, gApp->assetManagerRef);
    return gApp->audio.open() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL JNI_FN(nativeRestartAudio)(JNIEnv*, jobject) {
    if (!gApp) return;
    gApp->audio.close();
    gApp->audio.open();
}

JNIEXPORT void JNICALL JNI_FN(nativeLoadSamples)(JNIEnv*, jobject, jint layers) {
    if (!gApp) return;
    App& app = *gApp;
    app.cancelLoad = true;
    if (app.loader.joinable()) app.loader.join();
    app.cancelLoad = false;
    app.loadState = 1;
    app.loadProgress = 0.f;
    app.loader = std::thread([&app, layers] {
        // Libera o banco atual antes, para nao ter os dois na memoria ao mesmo tempo.
        unloadBank(app);
        std::string err;
        auto bank = loadSampleBank(layers, readAsset, &app.loadProgress, &app.cancelLoad, &err);
        if (!bank) {
            if (!app.cancelLoad) LOGE("%s", err.c_str());
            app.loadState = app.cancelLoad ? 0 : 3;
            return;
        }
        std::lock_guard<std::mutex> lock(app.bankMutex);
        app.bank = std::move(bank);
        app.bankBytes = static_cast<long long>(app.bank->bytes());
        app.engine.setBank(app.bank.get());
        app.loadState = 2;
        LOGI("samples carregados: %d camadas, %.1f MB", layers, app.bankBytes / 1e6);
    });
}

JNIEXPORT jint JNICALL JNI_FN(nativeOpenMidiPort)(JNIEnv* env, jobject, jobject midiDevice, jint portNumber) {
    if (!gApp) return -1;
    AMidiDevice* device = nullptr;
    if (AMidiDevice_fromJava(env, midiDevice, &device) != AMEDIA_OK || !device) return -1;
    AMidiOutputPort* port = nullptr;
    if (AMidiOutputPort_open(device, portNumber, &port) != AMEDIA_OK || !port) {
        AMidiDevice_release(device);
        return -1;
    }
    const int slot = gApp->audio.attachPort(device, port);
    if (slot < 0) {
        AMidiOutputPort_close(port);
        AMidiDevice_release(device);
    }
    return slot;
}

JNIEXPORT void JNICALL JNI_FN(nativeCloseMidiPort)(JNIEnv*, jobject, jint slot) {
    if (!gApp || slot < 0 || slot >= kMaxPorts) return;
    AMidiOutputPort* port = nullptr;
    AMidiDevice* device = nullptr;
    gApp->audio.detachPort(slot, &port, &device);
    if (port) AMidiOutputPort_close(port);
    if (device) AMidiDevice_release(device);
    gApp->engine.postMidi(0xB0, 123, 0);  // solta notas que ficaram presas
}

JNIEXPORT void JNICALL JNI_FN(nativeSendMidi)(JNIEnv*, jobject, jint status, jint d1, jint d2) {
    if (gApp) gApp->engine.postMidi(status, d1, d2);
}

JNIEXPORT void JNICALL JNI_FN(nativeSetParams)(JNIEnv*, jobject, jfloat volume, jfloat reverb,
                                               jfloat velocityCurve, jint transpose, jfloat tuneHz) {
    if (!gApp) return;
    PianoEngine& e = gApp->engine;
    e.volume = volume;
    e.reverbMix = reverb;
    e.velocityCurve = velocityCurve;
    e.transpose = transpose;
    e.tuneHz = tuneHz;
}

JNIEXPORT void JNICALL JNI_FN(nativeGetStats)(JNIEnv* env, jobject, jintArray out) {
    if (!gApp || env->GetArrayLength(out) < 16) return;
    jint s[16] = {0};
    gApp->audio.fillStats(s);
    PianoEngine& e = gApp->engine;
    s[8] = e.activeVoices.load();
    s[9] = e.lastNote.load();
    s[10] = e.lastVelocity.load();
    s[11] = static_cast<jint>(e.midiEvents.load());
    s[12] = e.sustainDown.load();
    s[13] = gApp->loadState.load();
    s[14] = static_cast<jint>(gApp->loadProgress.load() * 1000);
    s[15] = static_cast<jint>(gApp->bankBytes.load() / (1024 * 1024));
    env->SetIntArrayRegion(out, 0, 16, s);
}

}  // extern "C"
