#include "PianoEngine.h"

#include <algorithm>
#include <cmath>

#if defined(__aarch64__)
#include <cstdint>
#elif defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {

// Liga flush-to-zero: evita que denormais (caudas do reverb/envelope)
// derrubem a performance da thread de audio.
void enableFlushToZero() {
#if defined(__aarch64__)
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    asm volatile("msr fpcr, %0" ::"r"(fpcr | (1ull << 24)));
#elif defined(__SSE__)
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
}

inline float softClip(float x) {
    const float a = std::fabs(x);
    if (a <= 0.9f) return x;
    const float y = 0.9f + 0.1f * std::tanh((a - 0.9f) * 10.f);
    return x < 0.f ? -y : y;
}

// Tempo (s) para o abafador silenciar a corda; notas a partir de F6 nao tem abafador.
float releaseSeconds(int key) {
    if (key >= 89) return 3.5f;
    const float t = std::clamp((key - 21) / 67.f, 0.f, 1.f);
    return 0.55f + t * (0.18f - 0.55f);
}

}  // namespace

void PianoEngine::prepare(float sampleRate) {
    sampleRate_ = sampleRate > 0.f ? sampleRate : 48000.f;
    reverb_.setSampleRate(sampleRate_);
    limiterGain_ = 1.f;
    limiterRelease_ = decayCoef(1.f);  // recupera 90% do ganho em ~0,3 s
    for (auto& v : voices_) v = Voice{};
}

float PianoEngine::decayCoef(float seconds) const {
    // Cai 60 dB em 'seconds'.
    return std::pow(0.001f, 1.f / (std::max(seconds, 0.001f) * sampleRate_));
}

bool PianoEngine::postMidi(uint8_t status, uint8_t d1, uint8_t d2) {
    const uint32_t head = qHead_.load(std::memory_order_relaxed);
    const uint32_t next = (head + 1) % kQueueSize;
    if (next == qTail_.load(std::memory_order_acquire)) return false;
    queue_[head] = status | (d1 << 8) | (d2 << 16);
    qHead_.store(next, std::memory_order_release);
    return true;
}

void PianoEngine::drainQueue() {
    uint32_t tail = qTail_.load(std::memory_order_relaxed);
    while (tail != qHead_.load(std::memory_order_acquire)) {
        const uint32_t m = queue_[tail];
        handleMidi(m & 0xFF, (m >> 8) & 0x7F, (m >> 16) & 0x7F);
        tail = (tail + 1) % kQueueSize;
    }
    qTail_.store(tail, std::memory_order_release);
}

void PianoEngine::syncBank() {
    const SampleBank* wanted = requestedBank_.load(std::memory_order_acquire);
    if (wanted != bank_) {
        for (auto& v : voices_) v.state = State::Idle;  // as vozes apontam para o banco antigo
        bank_ = wanted;
        bankInUse_.store(bank_, std::memory_order_release);
    }
}

void PianoEngine::handleMidi(uint8_t status, uint8_t d1, uint8_t d2) {
    const uint8_t type = status & 0xF0;
    switch (type) {
        case 0x90:
            if (d2 > 0) { noteOn(d1, d2); break; }
            [[fallthrough]];
        case 0x80:
            noteOff(d1);
            break;
        case 0xB0:
            switch (d1) {
                case 64: setSustain(d2 >= 64); break;
                case 67: softPedal_ = d2 >= 64; break;
                case 7: channelVolume_ = (d2 / 127.f) * (d2 / 127.f); break;
                case 120: allNotesOff(true); break;
                case 121: setSustain(false); softPedal_ = false; channelVolume_ = 1.f; break;
                case 123: allNotesOff(false); break;
                default: break;
            }
            break;
        default:
            break;
    }
    midiEvents.fetch_add(1, std::memory_order_relaxed);
}

PianoEngine::Voice* PianoEngine::allocateVoice() {
    int active = 0;
    Voice* idle = nullptr;
    Voice* oldestReleasing = nullptr;
    Voice* oldestPlaying = nullptr;
    Voice* quietest = nullptr;
    for (auto& v : voices_) {
        if (v.state == State::Idle) { if (!idle) idle = &v; continue; }
        if (!quietest || v.env * v.gain < quietest->env * quietest->gain) quietest = &v;
        if (v.state == State::Fading) continue;
        ++active;
        if (v.state == State::Releasing) {
            if (!oldestReleasing || v.age < oldestReleasing->age) oldestReleasing = &v;
        } else if (!oldestPlaying || v.age < oldestPlaying->age) {
            oldestPlaying = &v;
        }
    }
    // Limite de polifonia: some rapidamente com a voz mais antiga (sem clique).
    if (active >= (lightMode.load(std::memory_order_relaxed) ? kMaxActiveVoicesLight : kMaxActiveVoices)) {
        Voice* victim = oldestReleasing ? oldestReleasing : oldestPlaying;
        if (victim) fade(*victim, 0.02f);
    }
    return idle ? idle : quietest;
}

void PianoEngine::noteOn(int key, int velocity) {
    lastNote.store(key, std::memory_order_relaxed);
    lastVelocity.store(velocity, std::memory_order_relaxed);
    if (!bank_) return;

    const int soundKey = std::clamp(key + transpose.load(std::memory_order_relaxed), 0, 127);
    const float curve = std::clamp(velocityCurve.load(std::memory_order_relaxed), 0.2f, 4.f);
    float vel = 1.f + 126.f * std::pow((std::clamp(velocity, 1, 127) - 1) / 126.f, curve);
    if (softPedal_) vel *= 0.8f;

    float layerGain = 1.f;
    const Sample* s = bank_->find(soundKey, vel, &layerGain);
    if (!s || !s->data) return;

    // Corda golpeada de novo: a voz anterior da mesma nota se dissolve.
    for (auto& v : voices_) {
        if (v.state != State::Idle && v.state != State::Fading && v.soundKey == soundKey) fade(v, 0.35f);
    }

    Voice* v = allocateVoice();
    if (!v) return;
    const float cents = 1200.f * std::log2(tuneHz.load(std::memory_order_relaxed) / 440.f);
    v->state = State::Playing;
    v->held = true;
    v->midiKey = static_cast<uint8_t>(key);
    v->soundKey = static_cast<uint8_t>(soundKey);
    v->sample = s;
    v->pos = 0.0;
    v->inc = std::pow(2.0, (soundKey - s->rootKey + cents / 100.0) / 12.0) * s->sampleRate / sampleRate_;
    v->gain = layerGain * (1.f / 32768.f);
    v->env = 1.f;
    v->coef = 1.f;
    // Envelope do SFZ (Rhodes): volume maximo durante o hold, depois decai ate o nivel de sustain.
    v->holdFrames = static_cast<int32_t>(s->holdSec * sampleRate_);
    v->playCoef = s->decaySec > 0.f
                      ? std::pow(s->sustainLevel, 1.f / (s->decaySec * sampleRate_))
                      : 1.f;
    v->age = ++ageCounter_;
}

void PianoEngine::noteOff(int key) {
    for (auto& v : voices_) {
        if (v.state == State::Playing && v.held && v.midiKey == key) {
            v.held = false;
            if (!sustain_) release(v);
        }
    }
}

void PianoEngine::setSustain(bool down) {
    sustain_ = down;
    sustainDown.store(down, std::memory_order_relaxed);
    if (!down) {
        for (auto& v : voices_)
            if (v.state == State::Playing && !v.held) release(v);
    }
}

void PianoEngine::allNotesOff(bool hard) {
    sustain_ = false;
    sustainDown.store(false, std::memory_order_relaxed);
    for (auto& v : voices_) {
        if (v.state == State::Idle) continue;
        v.held = false;
        if (hard) fade(v, 0.03f); else if (v.state == State::Playing) release(v);
    }
}

void PianoEngine::release(Voice& v) {
    v.state = State::Releasing;
    const float rel = v.sample && v.sample->releaseSec > 0.f ? v.sample->releaseSec : releaseSeconds(v.soundKey);
    v.coef = decayCoef(rel);
}

void PianoEngine::fade(Voice& v, float seconds) {
    const float c = decayCoef(seconds);
    if (v.state != State::Fading || c < v.coef) v.coef = c;
    v.state = State::Fading;
    v.held = false;
}

template <bool kCubic>
void PianoEngine::renderVoice(Voice& v, float* l, float* r, int n) {
    const Sample& s = *v.sample;
    const int16_t* data = s.data;
    const uint32_t last = s.frames > 3 ? s.frames - 3 : 0;
    const uint32_t loopEnd = s.loopEnd;
    const double loopLen = static_cast<double>(s.loopEnd) - s.loopStart;
    double pos = v.pos;
    const double inc = v.inc;
    float env = v.env;
    const float coef = v.coef;
    const float gain = v.gain;

    for (int i = 0; i < n; ++i) {
        if (loopEnd && pos >= loopEnd) pos -= loopLen;  // volta ao inicio do loop
        const uint32_t idx = static_cast<uint32_t>(pos);
        if (idx >= last) { v.state = State::Idle; break; }
        const float f = static_cast<float>(pos - idx);
        const int16_t* p = data + 2 * idx;
        const int16_t* pm = idx ? p - 2 : p;
        (void)pm;
        const float g = env * gain;
        if constexpr (kCubic) {
            // Interpolacao cubica de Hermite (4 pontos) por canal.
            for (int ch = 0; ch < 2; ++ch) {
                const float x0 = pm[ch], x1 = p[ch], x2 = p[2 + ch], x3 = p[4 + ch];
                const float c1 = 0.5f * (x2 - x0);
                const float c2 = x0 - 2.5f * x1 + 2.f * x2 - 0.5f * x3;
                const float c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
                const float y = ((c3 * f + c2) * f + c1) * f + x1;
                (ch == 0 ? l : r)[i] += y * g;
            }
        } else {
            // Linear: ~3x mais barata, diferenca quase inaudivel (no maximo 1 semitom de transposicao).
            l[i] += (p[0] + f * (p[2] - p[0])) * g;
            r[i] += (p[1] + f * (p[3] - p[1])) * g;
        }
        env *= coef;
        pos += inc;
    }
    v.pos = pos;
    v.env = env;
    if (env < 1e-4f) v.state = State::Idle;
}

void PianoEngine::render(float* out, int frames) {
    enableFlushToZero();
    syncBank();
    drainQueue();

    const float wet = std::clamp(reverbMix.load(std::memory_order_relaxed), 0.f, 1.f);
    const float master = std::clamp(volume.load(std::memory_order_relaxed), 0.f, 1.5f) * 0.6f * channelVolume_;

    for (int offset = 0; offset < frames; offset += kBlock) {
        const int n = std::min(kBlock, frames - offset);
        std::fill(mixL_, mixL_ + n, 0.f);
        std::fill(mixR_, mixR_ + n, 0.f);
        const bool light = lightMode.load(std::memory_order_relaxed);
        for (auto& v : voices_) {
            if (v.state == State::Idle) continue;
            if (v.state == State::Playing && v.holdFrames >= 0) {
                v.holdFrames -= n;
                if (v.holdFrames < 0) v.coef = v.playCoef;
            }
            if (light) renderVoice<false>(v, mixL_, mixR_, n);
            else renderVoice<true>(v, mixL_, mixR_, n);
        }
        if (wet > 0.001f) reverb_.process(mixL_, mixR_, mixL_, mixR_, n, wet);
        float* o = out + 2 * offset;
        for (int i = 0; i < n; ++i) {
            float l = mixL_[i] * master, r = mixR_[i] * master;
            // Limitador: reduz o ganho rapido em picos e recupera devagar.
            const float peak = std::fmax(std::fabs(l), std::fabs(r)) * limiterGain_;
            if (peak > 0.8f) limiterGain_ *= 0.8f / peak;
            else limiterGain_ = 1.f - (1.f - limiterGain_) * limiterRelease_;
            o[2 * i] = softClip(l * limiterGain_);
            o[2 * i + 1] = softClip(r * limiterGain_);
        }
    }

    int count = 0;
    for (const auto& v : voices_) count += v.state != State::Idle;
    activeVoices.store(count, std::memory_order_relaxed);
    renderCount_.fetch_add(1, std::memory_order_release);
}
