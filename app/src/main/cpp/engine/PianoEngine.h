#pragma once

#include <atomic>
#include <cstdint>

#include "Reverb.h"
#include "SampleBank.h"

// Motor do piano. render() e handleMidi() rodam na thread de audio e nao
// alocam memoria nem usam locks. Os demais metodos podem ser chamados de
// qualquer thread.
class PianoEngine {
public:
    static constexpr int kPoolVoices = 128;
    static constexpr int kMaxActiveVoices = 72;
    static constexpr int kBlock = 128;

    // Prepara para a taxa de amostragem do dispositivo (nunca durante render).
    void prepare(float sampleRate);

    void render(float* interleavedStereo, int frames);
    void handleMidi(uint8_t status, uint8_t d1, uint8_t d2);

    // Enfileira uma mensagem vinda de outra thread (UI); e aplicada no proximo bloco.
    bool postMidi(uint8_t status, uint8_t d1, uint8_t d2);

    // O dono do banco e externo. Depois de trocar, aguarde bankInUse() deixar
    // de apontar para o banco antigo antes de libera-lo.
    void setBank(const SampleBank* bank) { requestedBank_.store(bank); }
    const SampleBank* bankInUse() const { return bankInUse_.load(); }
    uint64_t renderCount() const { return renderCount_.load(); }

    // Parametros (thread-safe).
    std::atomic<float> volume{0.8f};        // 0..1.5
    std::atomic<float> reverbMix{0.2f};     // 0..1
    std::atomic<float> velocityCurve{1.f};  // expoente: <1 = toque leve, >1 = pesado
    std::atomic<float> tuneHz{440.f};       // afinacao de A4
    std::atomic<int> transpose{0};          // semitons

    // Estatisticas (leitura pela UI).
    std::atomic<int> activeVoices{0};
    std::atomic<int> lastNote{-1};
    std::atomic<int> lastVelocity{0};
    std::atomic<uint32_t> midiEvents{0};
    std::atomic<bool> sustainDown{false};

private:
    enum class State : uint8_t { Idle, Playing, Releasing, Fading };

    struct Voice {
        State state = State::Idle;
        bool held = false;
        uint8_t midiKey = 0;
        uint8_t soundKey = 0;
        const Sample* sample = nullptr;
        double pos = 0.0;
        double inc = 1.0;
        float gain = 0.f;
        float env = 1.f;
        float coef = 1.f;
        uint32_t age = 0;
    };

    void noteOn(int key, int velocity);
    void noteOff(int key);
    void setSustain(bool down);
    void allNotesOff(bool hard);
    void release(Voice& v);
    void fade(Voice& v, float seconds);
    Voice* allocateVoice();
    float decayCoef(float seconds) const;
    void renderVoice(Voice& v, float* l, float* r, int n);
    void drainQueue();
    void syncBank();

    float sampleRate_ = 48000.f;
    Voice voices_[kPoolVoices];
    uint32_t ageCounter_ = 0;
    bool sustain_ = false;
    bool softPedal_ = false;
    float channelVolume_ = 1.f;

    const SampleBank* bank_ = nullptr;
    std::atomic<const SampleBank*> requestedBank_{nullptr};
    std::atomic<const SampleBank*> bankInUse_{nullptr};
    std::atomic<uint64_t> renderCount_{0};

    Reverb reverb_;
    float limiterGain_ = 1.f;
    float limiterRelease_ = 0.9999f;
    float mixL_[kBlock];
    float mixR_[kBlock];

    // Fila SPSC de mensagens MIDI vindas da UI.
    static constexpr uint32_t kQueueSize = 256;
    uint32_t queue_[kQueueSize];
    std::atomic<uint32_t> qHead_{0};
    std::atomic<uint32_t> qTail_{0};
};
