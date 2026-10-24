// Ponte JNI: saida de audio (Oboe/AAudio), entrada MIDI nativa (AMidi) e
// carregamento dos samples. As mensagens MIDI sao lidas DENTRO do callback de
// audio, sem troca de thread: a nota entra no proximo buffer a ser tocado.

#include <amidi/AMidi.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <oboe/Oboe.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../engine/MidiParser.h"
#include "../engine/PianoEngine.h"

#define LOG_TAG "PianoMidi"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr int kMaxPorts = 8;
constexpr int kStatsSize = 20;

struct MidiSlot {
    std::atomic<AMidiOutputPort*> port{nullptr};
    AMidiDevice* device = nullptr;
    MidiParser parser;  // usado so pela thread de audio
};

// Combinacoes testadas na busca automatica. Cada aparelho libera o caminho
// rapido de audio para uma combinacao diferente (Xiaomi/MediaTek variam muito).
struct Route {
    oboe::AudioApi api;
    oboe::Usage usage;
};
constexpr Route kRoutes[] = {
    {oboe::AudioApi::AAudio, oboe::Usage::Game},
    {oboe::AudioApi::AAudio, oboe::Usage::Media},
    {oboe::AudioApi::OpenSLES, oboe::Usage::Media},
    {oboe::AudioApi::OpenSLES, oboe::Usage::Game},
};
constexpr int kNumRoutes = sizeof(kRoutes) / sizeof(kRoutes[0]);

int64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

// O OboeExtensions::isMMapUsed trata qualquer stream como AAudio; com OpenSL ES
// isso derruba o app. So consulta quando o stream e realmente AAudio.
bool isMMap(oboe::AudioStream* stream) {
    return stream && stream->getAudioApi() == oboe::AudioApi::AAudio &&
           oboe::OboeExtensions::isMMapUsed(stream);
}

void configureBuilder(oboe::AudioStreamBuilder& b, const Route& route) {
    b.setDirection(oboe::Direction::Output)
        ->setAudioApi(route.api)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        ->setUsage(route.usage)
        ->setContentType(oboe::ContentType::Music)
        ->setFormatConversionAllowed(true)
        ->setChannelConversionAllowed(true);
}

// Toca silencio e registra o instante de cada callback: o intervalo real entre
// callbacks revela se o sistema deu o caminho rapido (~2-5 ms) ou o comum (~20 ms).
class ProbeCallback : public oboe::AudioStreamDataCallback {
public:
    static constexpr int kMax = 1024;
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* s, void* data, int32_t frames) override {
        std::memset(data, 0, static_cast<size_t>(frames) * s->getBytesPerFrame());
        const int i = count.load(std::memory_order_relaxed);
        if (i < kMax) {
            times[i] = nowNs();
            count.store(i + 1, std::memory_order_release);
        }
        return oboe::DataCallbackResult::Continue;
    }
    int64_t times[kMax];
    std::atomic<int> count{0};
};

// Retorna o intervalo p90 entre callbacks em ms, ou -1 se a rota falhar.
double probeRoute(const Route& route) {
    ProbeCallback cb;
    oboe::AudioStreamBuilder b;
    configureBuilder(b, route);
    b.setDataCallback(&cb);
    std::shared_ptr<oboe::AudioStream> stream;
    if (b.openStream(stream) != oboe::Result::OK) return -1;
    if (stream->getAudioApi() != route.api) { stream->close(); return -1; }
    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    if (stream->requestStart() != oboe::Result::OK) { stream->close(); return -1; }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stream->stop();
    stream->close();
    const int n = cb.count.load(std::memory_order_acquire);
    std::vector<double> gaps;
    for (int i = 11; i < n; ++i) gaps.push_back((cb.times[i] - cb.times[i - 1]) / 1e6);  // ignora o inicio
    if (gaps.size() < 5) return -1;
    std::sort(gaps.begin(), gaps.end());
    return gaps[gaps.size() * 9 / 10];
}

class AudioOutput : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
public:
    explicit AudioOutput(PianoEngine& engine) : engine_(engine) {}

    void setConfig(int route, int bufferBursts) {
        std::lock_guard<std::mutex> lock(streamMutex_);
        route_ = std::clamp(route, 0, kNumRoutes - 1);
        bufferBursts_ = std::clamp(bufferBursts, 1, 4);
    }

    bool open() {
        std::lock_guard<std::mutex> lock(streamMutex_);
        return openLocked();
    }

    void close() {
        std::lock_guard<std::mutex> lock(streamMutex_);
        closeLocked();
    }

    // Testa todas as rotas e reabre com a de menor intervalo entre callbacks.
    // gapsTenthMs recebe o resultado de cada rota (decimos de ms, -1 = falhou).
    int probe(int* gapsTenthMs) {
        std::lock_guard<std::mutex> lock(streamMutex_);
        probing_ = true;
        closeLocked();
        int best = -1;
        double bestGap = 1e9;
        for (int i = 0; i < kNumRoutes; ++i) {
            const double gap = probeRoute(kRoutes[i]);
            gapsTenthMs[i] = gap < 0 ? -1 : static_cast<int>(gap * 10);
            LOGI("rota %d: intervalo p90 %.2f ms", i, gap);
            // So troca se for claramente melhor (empate favorece a rota anterior).
            if (gap > 0 && gap < bestGap - 0.5) { bestGap = gap; best = i; }
        }
        if (best >= 0) route_ = best;
        openLocked();
        probing_ = false;
        return best;
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) override {
        measureGap();
        tune(stream);
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
        s[18] = probing_.load();
        std::unique_lock<std::mutex> lock(streamMutex_, std::try_to_lock);
        if (!lock.owns_lock()) return;  // busca em andamento: nao trava a UI
        s[0] = stream_ ? stream_->getSampleRate() : 0;
        s[1] = stream_ ? stream_->getFramesPerBurst() : 0;
        s[2] = stream_ ? stream_->getBufferSizeInFrames() : 0;
        auto xruns = stream_ ? stream_->getXRunCount() : oboe::ResultWithValue<int32_t>(oboe::Result::ErrorNull);
        s[3] = xruns ? xruns.value() : -1;
        s[4] = stream_ && stream_->getSharingMode() == oboe::SharingMode::Exclusive;
        s[5] = isMMap(stream_.get());
        auto latency = stream_ ? stream_->calculateLatencyMillis() : oboe::ResultWithValue<double>(oboe::Result::ErrorNull);
        s[6] = latency ? static_cast<jint>(latency.value() * 10) : -1;  // decimos de ms
        s[7] = stream_ && stream_->getPerformanceMode() == oboe::PerformanceMode::LowLatency;
        s[16] = route_;
        s[17] = stream_ && stream_->getAudioApi() == oboe::AudioApi::AAudio;
        s[19] = gapTenthMs_.load(std::memory_order_relaxed);
    }

private:
    static constexpr int kScratchFrames = 256;

    void closeLocked() {
        wantRunning_ = false;
        if (stream_) {
            stream_->stop();
            stream_->close();
            stream_.reset();
        }
    }

    bool openLocked() {
        wantRunning_ = true;
        oboe::AudioStreamBuilder b;
        configureBuilder(b, kRoutes[route_]);
        b.setDataCallback(this)->setErrorCallback(this);
        std::shared_ptr<oboe::AudioStream> stream;
        oboe::Result r = b.openStream(stream);
        if (r != oboe::Result::OK) {
            LOGE("falha ao abrir audio: %s", oboe::convertToText(r));
            return false;
        }
        engine_.prepare(static_cast<float>(stream->getSampleRate()));
        // Pede ao sistema para subir o clock da CPU quando a carga aumenta (acordes, pedal).
        stream->setPerformanceHintEnabled(true);
        // Comeca com o minimo escolhido; tune() aumenta so se houver underruns.
        stream->setBufferSizeInFrames(stream->getFramesPerBurst() * bufferBursts_);
        lastXruns_ = 0;
        lastCallbackNs_ = 0;
        windowMaxNs_ = 0;
        windowStartNs_ = 0;
        r = stream->requestStart();
        if (r != oboe::Result::OK) {
            LOGE("falha ao iniciar audio: %s", oboe::convertToText(r));
            stream->close();
            return false;
        }
        stream_ = stream;
        LOGI("audio: rota %d, %d Hz, burst %d, buffer %d, mmap %d", route_, stream->getSampleRate(),
             stream->getFramesPerBurst(), stream->getBufferSizeInFrames(),
             isMMap(stream.get()));
        return true;
    }

    // Aumenta o buffer em 1 burst a cada underrun novo.
    void tune(oboe::AudioStream* stream) {
        auto xruns = stream->getXRunCount();
        if (!xruns || xruns.value() <= lastXruns_) return;
        lastXruns_ = xruns.value();
        const int32_t size = stream->getBufferSizeInFrames() + stream->getFramesPerBurst();
        if (size <= stream->getBufferCapacityInFrames()) stream->setBufferSizeInFrames(size);
    }

    // Maior intervalo entre callbacks em janelas de 1 s (mostrado na tela).
    void measureGap() {
        const int64_t now = nowNs();
        if (lastCallbackNs_ > 0) windowMaxNs_ = std::max(windowMaxNs_, now - lastCallbackNs_);
        lastCallbackNs_ = now;
        if (now - windowStartNs_ > 1000000000LL) {
            if (windowStartNs_ > 0) gapTenthMs_.store(static_cast<int>(windowMaxNs_ / 100000), std::memory_order_relaxed);
            windowStartNs_ = now;
            windowMaxNs_ = 0;
        }
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
    bool wantRunning_ = false;
    std::atomic<bool> probing_{false};
    int route_ = 0;
    int bufferBursts_ = 2;
    int32_t lastXruns_ = 0;
    int64_t lastCallbackNs_ = 0;
    int64_t windowStartNs_ = 0;
    int64_t windowMaxNs_ = 0;
    std::atomic<int> gapTenthMs_{0};
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

JNIEXPORT jboolean JNICALL JNI_FN(nativeCreate)(JNIEnv* env, jobject, jobject assetManager,
                                                jint nativeSampleRate, jint nativeBurst,
                                                jint route, jint bufferBursts) {
    if (gApp) return JNI_TRUE;
    // Valores nativos do aparelho: sem eles o OpenSL ES nunca ganha o caminho rapido.
    if (nativeSampleRate > 0) oboe::DefaultStreamValues::SampleRate = nativeSampleRate;
    if (nativeBurst > 0) oboe::DefaultStreamValues::FramesPerBurst = nativeBurst;
    gApp = new App();
    gApp->audio.setConfig(route, bufferBursts);
    gApp->assetManagerRef = env->NewGlobalRef(assetManager);
    gApp->assets = AAssetManager_fromJava(env, gApp->assetManagerRef);
    return gApp->audio.open() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL JNI_FN(nativeConfigureAudio)(JNIEnv*, jobject, jint route, jint bufferBursts) {
    if (!gApp) return;
    gApp->audio.close();
    gApp->audio.setConfig(route, bufferBursts);
    gApp->audio.open();
}

// Bloqueia ~2,5 s: chame fora da thread principal. Retorna [melhor rota, intervalo de cada rota].
JNIEXPORT jintArray JNICALL JNI_FN(nativeProbeAudio)(JNIEnv* env, jobject) {
    jint result[1 + kNumRoutes] = {-1};
    if (gApp) result[0] = gApp->audio.probe(result + 1);
    jintArray arr = env->NewIntArray(1 + kNumRoutes);
    env->SetIntArrayRegion(arr, 0, 1 + kNumRoutes, result);
    return arr;
}

JNIEXPORT void JNICALL JNI_FN(nativeSetLightMode)(JNIEnv*, jobject, jboolean light) {
    if (gApp) gApp->engine.lightMode = light;
}

// instrument: 0 = piano (Salamander, 4 ou 8 camadas), 1 = Rhodes (jRhodes3c).
// So o instrumento ativo fica na memoria.
JNIEXPORT void JNICALL JNI_FN(nativeLoadInstrument)(JNIEnv*, jobject, jint instrument, jint layers) {
    if (!gApp) return;
    App& app = *gApp;
    app.cancelLoad = true;
    if (app.loader.joinable()) app.loader.join();
    app.cancelLoad = false;
    app.loadState = 1;
    app.loadProgress = 0.f;
    app.loader = std::thread([&app, instrument, layers] {
        // Libera o banco atual antes, para nao ter os dois na memoria ao mesmo tempo.
        unloadBank(app);
        std::string err;
        auto bank = instrument == 1
                        ? loadZoneBank("rhodes", 1.4f, readAsset, &app.loadProgress, &app.cancelLoad, &err)
                        : loadSampleBank(layers, readAsset, &app.loadProgress, &app.cancelLoad, &err);
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
        LOGI("instrumento %d carregado: %.1f MB", instrument, app.bankBytes / 1e6);
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
    if (!gApp || env->GetArrayLength(out) < kStatsSize) return;
    jint s[kStatsSize] = {0};
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
    env->SetIntArrayRegion(out, 0, kStatsSize, s);
}

}  // extern "C"
