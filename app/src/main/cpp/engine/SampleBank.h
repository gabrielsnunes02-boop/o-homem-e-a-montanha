#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Um sample decodificado (int16 estereo intercalado).
struct Sample {
    Sample() = default;
    ~Sample();
    Sample(const Sample&) = delete;
    Sample& operator=(const Sample&) = delete;

    int16_t* data = nullptr;  // alocado pelo stb_vorbis (malloc)
    uint32_t frames = 0;
    float sampleRate = 48000.f;
    int rootKey = 60;
    float loudnessDb = -20.f;  // pico do ataque, usado para suavizar a curva de velocity

    // Instrumentos com loop (Rhodes). loopEnd == 0: sem loop (toca ate o fim).
    uint32_t loopStart = 0;
    uint32_t loopEnd = 0;     // exclusivo
    float holdSec = 0.f;      // tempo em volume maximo antes do decaimento
    float decaySec = 0.f;     // 0 = sem decaimento enquanto a tecla esta presa
    float sustainLevel = 1.f; // nivel alvo do decaimento (0..1)
    float releaseSec = -1.f;  // <0: abafador do piano (depende da nota)
};

// Conjunto de samples: 30 regioes (de A0 a C8 em tercas menores) x N camadas.
class SampleBank {
public:
    static constexpr int kRegions = 30;
    static constexpr int kFirstRoot = 21;  // A0

    struct Layer {
        int source;  // numero da camada no Salamander (v1..v16)
        int topVel;  // maior velocity atendida por esta camada
        std::array<Sample, kRegions> samples;
    };

    // Camadas usadas em cada modo de qualidade (4 ou 8 camadas).
    static std::vector<std::pair<int, int>> layout(int numLayers);

    static int regionForKey(int key);
    static std::string fileName(int region, int sourceLayer);

    // Escolhe o sample para (tecla, velocity) e devolve o ganho linear que
    // deixa a dinamica continua entre camadas vizinhas.
    const Sample* find(int key, float velocity, float* gain) const;

    // Instrumento por zonas (mapa SFZ). Se houver zonas, find() usa elas.
    struct Zone {
        int loKey, hiKey, loVel, hiVel;
        const Sample* sample;
    };

    std::vector<std::unique_ptr<Layer>> layers;
    std::vector<Zone> zones;
    std::vector<std::unique_ptr<Sample>> zoneSamples;
    float gain = 1.f;  // ajuste de volume do instrumento inteiro
    size_t bytes() const;
};

using FileReader = std::function<bool(const std::string& name, std::vector<uint8_t>& out)>;

// Decodifica todos os samples em paralelo. 'progress' vai de 0 a 1.
// Retorna nullptr em caso de erro ou cancelamento.
std::unique_ptr<SampleBank> loadSampleBank(int numLayers, const FileReader& reader,
                                           std::atomic<float>* progress,
                                           const std::atomic<bool>* cancel,
                                           std::string* error);

// Carrega um instrumento por zonas a partir de '<dir>/regions.txt'.
std::unique_ptr<SampleBank> loadZoneBank(const std::string& dir, float gain, const FileReader& reader,
                                         std::atomic<float>* progress,
                                         const std::atomic<bool>* cancel,
                                         std::string* error);
