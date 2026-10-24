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

    std::vector<std::unique_ptr<Layer>> layers;
    size_t bytes() const;
};

using FileReader = std::function<bool(const std::string& name, std::vector<uint8_t>& out)>;

// Decodifica todos os samples em paralelo. 'progress' vai de 0 a 1.
// Retorna nullptr em caso de erro ou cancelamento.
std::unique_ptr<SampleBank> loadSampleBank(int numLayers, const FileReader& reader,
                                           std::atomic<float>* progress,
                                           const std::atomic<bool>* cancel,
                                           std::string* error);
