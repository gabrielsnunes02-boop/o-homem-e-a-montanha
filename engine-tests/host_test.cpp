// Teste do motor no PC (sem Android): parser MIDI, carregamento, latencia de
// ataque, carga de CPU e uma demo renderizada em WAV.
//
//   cmake -S engine-tests -B build-host && cmake --build build-host
//   ./build-host/host_test app/src/main/assets demo.wav

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "../app/src/main/cpp/engine/MidiParser.h"
#include "../app/src/main/cpp/engine/PianoEngine.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FALHOU: %s (linha %d)\n", #cond, __LINE__); ++failures; } } while (0)

static void testParser() {
    MidiParser p;
    std::vector<uint32_t> got;
    auto emit = [&](uint8_t s, uint8_t a, uint8_t b) { got.push_back(s | a << 8 | b << 16); };
    // Note on, running status com clock (F8) intercalado, SysEx no meio e program change.
    const uint8_t bytes[] = {0x90, 60, 100, 0xF8, 62, 0x7F, 0xF0, 1, 2, 3, 0xF7, 0x80, 60, 0, 0xC0, 5, 0xB0, 64, 127};
    p.feed(bytes, 4, emit);
    p.feed(bytes + 4, sizeof(bytes) - 4, emit);
    CHECK(got.size() == 5);
    if (got.size() == 5) {
        CHECK(got[0] == (0x90u | 60 << 8 | 100 << 16));
        CHECK(got[1] == (0x90u | 62 << 8 | 127 << 16));
        CHECK(got[2] == (0x80u | 60 << 8));
        CHECK(got[3] == (0xC0u | 5 << 8));
        CHECK(got[4] == (0xB0u | 64 << 8 | 127 << 16));
    }
}

static void writeWav(const std::string& path, const std::vector<float>& s, int sr) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<char*>(&v), 4); };
    auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<char*>(&v), 2); };
    const uint32_t bytes = static_cast<uint32_t>(s.size() * 2);
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8);
    u32(16); u16(1); u16(2); u32(sr); u32(sr * 4); u16(4); u16(16);
    f.write("data", 4); u32(bytes);
    for (float x : s) { int16_t v = static_cast<int16_t>(std::lround(std::fmax(-1.f, std::fmin(1.f, x)) * 32767)); f.write(reinterpret_cast<char*>(&v), 2); }
}

int main(int argc, char** argv) {
    const std::string assets = argc > 1 ? argv[1] : "app/src/main/assets";
    const std::string wavOut = argc > 2 ? argv[2] : "";
    const int layers = argc > 3 ? std::atoi(argv[3]) : 4;
    testParser();

    FileReader reader = [&](const std::string& name, std::vector<uint8_t>& out) {
        std::ifstream f(assets + "/" + name, std::ios::binary);
        if (!f) return false;
        out.assign(std::istreambuf_iterator<char>(f), {});
        return true;
    };
    std::atomic<float> progress{0.f};
    std::string err;
    auto t0 = std::chrono::steady_clock::now();
    auto bank = loadSampleBank(layers, reader, &progress, nullptr, &err);
    const double loadS = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    CHECK(bank != nullptr);
    if (!bank) { std::printf("erro: %s\n", err.c_str()); return 1; }
    std::printf("banco %d camadas: %.1f MB, carregado em %.2f s\n", layers, bank->bytes() / 1e6, loadS);

    const int sr = 48000;
    auto engine = std::make_unique<PianoEngine>();
    engine->prepare(sr);
    engine->setBank(bank.get());
    std::vector<float> buf(2 * 4096);
    engine->render(buf.data(), 96);
    CHECK(engine->bankInUse() == bank.get());

    // Latencia de ataque: note-on no inicio do bloco -> primeiro sample audivel.
    for (int vel : {20, 64, 127}) {
        for (int key : {21, 60, 108}) {
            engine->handleMidi(0x90, key, vel);
            std::fill(buf.begin(), buf.end(), 0.f);
            engine->render(buf.data(), 4096);
            int first = -1;
            float peak = 0.f;
            for (int i = 0; i < 4096; ++i) {
                const float a = std::fmax(std::fabs(buf[2 * i]), std::fabs(buf[2 * i + 1]));
                peak = std::fmax(peak, a);
            }
            for (int i = 0; i < 4096 && first < 0; ++i)
                if (std::fmax(std::fabs(buf[2 * i]), std::fabs(buf[2 * i + 1])) > peak * 0.02f) first = i;
            std::printf("  ataque tecla %3d vel %3d: inicio do som em %.2f ms (pico %.3f)\n", key, vel, first * 1000.0 / sr, peak);
            CHECK(first >= 0 && first < sr * 0.003);  // som comeca em menos de 3 ms
            engine->handleMidi(0xB0, 120, 0);
            engine->render(buf.data(), 4096);
        }
    }

    // Velocity monotonica: mais forca => mais volume (sem degraus invertidos).
    {
        float prev = 0.f;
        int inversions = 0;
        for (int vel = 1; vel <= 127; vel += 2) {
            float gain = 0.f;
            const Sample* s = bank->find(60, static_cast<float>(vel), &gain);
            const float level = std::pow(10.f, s->loudnessDb / 20.f) * gain;
            if (level < prev * 0.97f) ++inversions;
            prev = level;
        }
        std::printf("  inversoes de dinamica na curva de velocity: %d\n", inversions);
        CHECK(inversions == 0);
    }

    // Estresse de CPU: pedal abaixado e 88 notas tocadas rapidamente (modo normal e leve).
    for (bool light : {false, true}) {
        engine->lightMode = light;
        engine->handleMidi(0xB0, 64, 127);
        const int seconds = 6;
        double renderTime = 0;
        int key = 21;
        float maxAbs = 0.f;
        bool finite = true;
        for (int b = 0; b < seconds * sr / 192; ++b) {
            if (b % 3 == 0) { engine->handleMidi(0x90, key, 40 + (key * 7) % 87); key = key >= 108 ? 21 : key + 1; }
            auto s = std::chrono::steady_clock::now();
            engine->render(buf.data(), 192);
            renderTime += std::chrono::duration<double>(std::chrono::steady_clock::now() - s).count();
            for (int i = 0; i < 384; ++i) { maxAbs = std::fmax(maxAbs, std::fabs(buf[i])); finite &= std::isfinite(buf[i]); }
        }
        std::printf("  estresse%s: %d vozes ativas, CPU %.2f%% de um nucleo (PC), pico %.3f\n", light ? " (modo leve)" : "",
                    engine->activeVoices.load(), 100.0 * renderTime / seconds, maxAbs);
        if (light) CHECK(engine->activeVoices.load() <= PianoEngine::kMaxActiveVoicesLight + 24);
        CHECK(finite);
        CHECK(maxAbs <= 1.0f);
        CHECK(engine->activeVoices.load() <= PianoEngine::kPoolVoices);
        engine->handleMidi(0xB0, 64, 0);
        engine->handleMidi(0xB0, 123, 0);
        for (int i = 0; i < 6 * sr / 192; ++i) engine->render(buf.data(), 192);  // agudos sem abafador soam ~3,5 s
        std::printf("  vozes apos soltar pedal: %d\n", engine->activeVoices.load());
        CHECK(engine->activeVoices.load() == 0);
    }
    engine->lightMode = false;

    // Troca de banco: vozes antigas sao descartadas.
    engine->handleMidi(0x90, 60, 90);
    engine->setBank(nullptr);
    engine->render(buf.data(), 64);
    CHECK(engine->bankInUse() == nullptr);
    CHECK(engine->activeVoices.load() == 0);
    engine->setBank(bank.get());

    if (!wavOut.empty()) {
        // Demo: escala, acorde com pedal, dinamica pp->ff e notas graves/agudas.
        struct Ev { double t; uint8_t s, a, b; };
        std::vector<Ev> evs;
        const int scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
        for (int i = 0; i < 8; ++i) { evs.push_back({i * 0.25, 0x90, (uint8_t)scale[i], 80}); evs.push_back({i * 0.25 + 0.22, 0x80, (uint8_t)scale[i], 0}); }
        evs.push_back({2.2, 0xB0, 64, 127});
        for (int k : {36, 48, 55, 64, 67, 72}) evs.push_back({2.3 + (k - 36) * 0.004, 0x90, (uint8_t)k, 85});
        for (int k : {36, 48, 55, 64, 67, 72}) evs.push_back({2.9, 0x80, (uint8_t)k, 0});
        evs.push_back({5.5, 0xB0, 64, 0});
        for (int i = 0; i < 8; ++i) { evs.push_back({6.0 + i * 0.3, 0x90, 67, (uint8_t)(10 + i * 16)}); evs.push_back({6.25 + i * 0.3, 0x80, 67, 0}); }
        for (int k : {21, 28, 33}) { evs.push_back({9.0, 0x90, (uint8_t)k, 100}); evs.push_back({11.0, 0x80, (uint8_t)k, 0}); }
        for (int i = 0; i < 12; ++i) { evs.push_back({11.5 + i * 0.09, 0x90, (uint8_t)(96 + i), 70}); evs.push_back({11.58 + i * 0.09, 0x80, (uint8_t)(96 + i), 0}); }
        std::sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
        std::vector<float> out;
        size_t e = 0;
        const int blk = 96;
        for (int f = 0; f < sr * 15; f += blk) {
            while (e < evs.size() && evs[e].t * sr <= f) { engine->handleMidi(evs[e].s, evs[e].a, evs[e].b); ++e; }
            engine->render(buf.data(), blk);
            out.insert(out.end(), buf.begin(), buf.begin() + 2 * blk);
        }
        writeWav(wavOut, out, sr);
        std::printf("demo gravada em %s\n", wavOut.c_str());
    }

    // ---- Rhodes (instrumento por zonas com loop) ----
    {
        t0 = std::chrono::steady_clock::now();
        auto rhodes = loadZoneBank("rhodes", 1.4f, reader, &progress, nullptr, &err);
        const double rs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        CHECK(rhodes != nullptr);
        if (!rhodes) { std::printf("erro: %s\n", err.c_str()); return 1; }
        std::printf("rhodes: %zu zonas, %.1f MB, carregado em %.2f s\n", rhodes->zones.size(), rhodes->bytes() / 1e6, rs);
        int looped = 0;
        for (const auto& z : rhodes->zones) looped += z.sample->loopEnd > 0;
        CHECK(looped > 50);

        engine->setBank(rhodes.get());
        engine->render(buf.data(), 64);
        CHECK(engine->bankInUse() == rhodes.get());

        // Nota grave segurada 6 s: o loop precisa sustentar alem do fim do sample.
        float gain = 0.f;
        const Sample* s29 = rhodes->find(29, 60.f, &gain);
        CHECK(s29 && s29->loopEnd > 0);
        engine->handleMidi(0x90, 29, 60);
        float lateRms = 0.f;
        bool finite = true;
        float peak = 0.f;
        for (int b = 0; b < 6 * sr / 480; ++b) {
            engine->render(buf.data(), 480);
            for (int i = 0; i < 960; ++i) { finite &= std::isfinite(buf[i]); peak = std::fmax(peak, std::fabs(buf[i])); }
            if (b == 6 * sr / 480 - 1) {
                for (int i = 0; i < 960; ++i) lateRms += buf[i] * buf[i];
                lateRms = std::sqrt(lateRms / 960);
            }
        }
        const float sampleSec = s29 ? s29->frames / s29->sampleRate : 0.f;
        std::printf("  rhodes F1 segurado 6 s (sample de %.1f s): rms final %.4f, pico %.3f, vozes %d\n",
                    sampleSec, lateRms, peak, engine->activeVoices.load());
        CHECK(finite);
        CHECK(lateRms > 1e-4f);
        CHECK(engine->activeVoices.load() == 1);
        engine->handleMidi(0x80, 29, 0);
        for (int b = 0; b < sr / 480; ++b) engine->render(buf.data(), 480);  // release de 0,3 s
        std::printf("  rhodes apos soltar (1 s): %d vozes\n", engine->activeVoices.load());
        CHECK(engine->activeVoices.load() == 0);

        // Todas as teclas e camadas tem zona.
        int missing = 0;
        for (int k = 21; k <= 108; ++k)
            for (int v : {1, 50, 80, 100, 127}) { float g; missing += rhodes->find(k, static_cast<float>(v), &g) == nullptr; }
        CHECK(missing == 0);

        if (!wavOut.empty()) {
            std::vector<float> out;
            struct Ev { double t; uint8_t s, a, b; };
            std::vector<Ev> evs;
            const int prog[4][4] = {{50, 53, 57, 60}, {55, 59, 62, 65}, {48, 52, 55, 59}, {45, 48, 52, 55}};
            for (int c = 0; c < 4; ++c) {
                for (int k : prog[c]) { evs.push_back({c * 1.6, 0x90, (uint8_t)k, (uint8_t)(60 + c * 15)}); evs.push_back({c * 1.6 + 1.5, 0x80, (uint8_t)k, 0}); }
                evs.push_back({c * 1.6, 0x90, (uint8_t)(prog[c][0] - 24), 90}); evs.push_back({c * 1.6 + 1.5, 0x80, (uint8_t)(prog[c][0] - 24), 0});
            }
            const int mel[] = {72, 74, 76, 79, 81, 79, 76, 74, 72};
            for (int i = 0; i < 9; ++i) { evs.push_back({6.6 + i * 0.22, 0x90, (uint8_t)mel[i], (uint8_t)(40 + i * 10)}); evs.push_back({6.6 + i * 0.22 + 0.2, 0x80, (uint8_t)mel[i], 0}); }
            std::sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
            size_t e = 0;
            for (int f = 0; f < sr * 10; f += 96) {
                while (e < evs.size() && evs[e].t * sr <= f) { engine->handleMidi(evs[e].s, evs[e].a, evs[e].b); ++e; }
                engine->render(buf.data(), 96);
                out.insert(out.end(), buf.begin(), buf.begin() + 192);
            }
            const std::string path = wavOut.substr(0, wavOut.size() - 4) + "-rhodes.wav";
            writeWav(path, out, sr);
            std::printf("demo rhodes gravada em %s\n", path.c_str());
        }
        engine->setBank(nullptr);
        engine->render(buf.data(), 64);
    }

    std::printf(failures ? "\n%d FALHA(S)\n" : "\nTODOS OS TESTES PASSARAM\n", failures);
    return failures ? 1 : 0;
}
