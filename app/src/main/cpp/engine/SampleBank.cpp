#include "SampleBank.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <thread>

#define STB_VORBIS_HEADER_ONLY
#include "../third_party/stb_vorbis.c"

namespace {

const char* const kNoteNames[SampleBank::kRegions] = {
    "A0", "C1", "Ds1", "Fs1", "A1", "C2", "Ds2", "Fs2", "A2", "C3",
    "Ds3", "Fs3", "A3", "C4", "Ds4", "Fs4", "A4", "C5", "Ds5", "Fs5",
    "A5", "C6", "Ds6", "Fs6", "A6", "C7", "Ds7", "Fs7", "A7", "C8"};

float measureLoudnessDb(const Sample& s) {
    const uint32_t n = std::min<uint32_t>(s.frames, static_cast<uint32_t>(s.sampleRate * 0.15f));
    int peak = 1;
    for (uint32_t i = 0; i < n * 2; ++i) peak = std::max(peak, std::abs(static_cast<int>(s.data[i])));
    return 20.f * std::log10(peak / 32768.f);
}

}  // namespace

Sample::~Sample() { std::free(data); }

std::vector<std::pair<int, int>> SampleBank::layout(int numLayers) {
    if (numLayers >= 8) {
        return {{1, 36}, {4, 46}, {6, 56}, {8, 72}, {10, 88}, {12, 104}, {14, 120}, {16, 127}};
    }
    return {{4, 50}, {8, 80}, {12, 108}, {16, 127}};
}

int SampleBank::regionForKey(int key) {
    const int r = static_cast<int>(std::lround((key - kFirstRoot) / 3.0));
    return std::clamp(r, 0, kRegions - 1);
}

std::string SampleBank::fileName(int region, int sourceLayer) {
    return std::string("samples/") + kNoteNames[region] + "v" + std::to_string(sourceLayer) + ".ogg";
}

const Sample* SampleBank::find(int key, float velocity, float* gain) const {
    if (layers.empty()) return nullptr;
    const int region = regionForKey(key);
    size_t i = 0;
    while (i + 1 < layers.size() && velocity > layers[i]->topVel) ++i;

    // Interpola a "loudness" alvo em dB entre a camada anterior e a atual,
    // assim nao ha degrau de volume ao cruzar o limite entre camadas.
    const Layer& cur = *layers[i];
    const float curDb = cur.samples[region].loudnessDb;
    const float prevTop = i > 0 ? static_cast<float>(layers[i - 1]->topVel) : 0.f;
    const float prevDb = i > 0 ? layers[i - 1]->samples[region].loudnessDb : curDb - 24.f;
    const float t = std::clamp((velocity - prevTop) / (cur.topVel - prevTop), 0.f, 1.f);
    const float targetDb = prevDb + t * (curDb - prevDb);
    const float gainDb = std::min(targetDb - curDb, 6.f);
    *gain = std::pow(10.f, gainDb / 20.f);
    return &cur.samples[region];
}

size_t SampleBank::bytes() const {
    size_t total = 0;
    for (const auto& l : layers)
        for (const auto& s : l->samples) total += s.frames * 2 * sizeof(int16_t);
    return total;
}

std::unique_ptr<SampleBank> loadSampleBank(int numLayers, const FileReader& reader,
                                           std::atomic<float>* progress,
                                           const std::atomic<bool>* cancel,
                                           std::string* error) {
    auto bank = std::make_unique<SampleBank>();
    struct Job { Sample* sample; int region; int source; };
    std::vector<Job> jobs;
    for (const auto& [source, top] : SampleBank::layout(numLayers)) {
        auto layer = std::make_unique<SampleBank::Layer>();
        layer->source = source;
        layer->topVel = top;
        for (int r = 0; r < SampleBank::kRegions; ++r) jobs.push_back({&layer->samples[r], r, source});
        bank->layers.push_back(std::move(layer));
    }

    std::atomic<size_t> next{0};
    std::atomic<size_t> done{0};
    std::atomic<bool> failed{false};
    std::mutex errorMutex;
    std::mutex readMutex;  // o leitor (AAssetManager) e acessado de forma serializada

    auto worker = [&] {
        std::vector<uint8_t> bytes;
        for (;;) {
            const size_t idx = next.fetch_add(1);
            if (idx >= jobs.size() || failed.load() || (cancel && cancel->load())) return;
            const Job& job = jobs[idx];
            const std::string name = SampleBank::fileName(job.region, job.source);
            bool ok;
            {
                std::lock_guard<std::mutex> lock(readMutex);
                ok = reader(name, bytes);
            }
            int channels = 0, rate = 0;
            short* pcm = nullptr;
            const int frames = ok ? stb_vorbis_decode_memory(bytes.data(), static_cast<int>(bytes.size()),
                                                            &channels, &rate, &pcm)
                                  : -1;
            if (frames <= 0 || channels != 2) {
                std::free(pcm);
                std::lock_guard<std::mutex> lock(errorMutex);
                if (error && !failed.load()) *error = "falha ao carregar " + name;
                failed = true;
                return;
            }
            Sample& s = *job.sample;
            s.data = pcm;
            s.frames = static_cast<uint32_t>(frames);
            s.sampleRate = static_cast<float>(rate);
            s.rootKey = SampleBank::kFirstRoot + 3 * job.region;
            s.loudnessDb = measureLoudnessDb(s);
            const size_t d = done.fetch_add(1) + 1;
            if (progress) progress->store(static_cast<float>(d) / jobs.size());
        }
    };

    const unsigned n = std::clamp(std::thread::hardware_concurrency(), 1u, 4u);
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < n; ++i) threads.emplace_back(worker);
    for (auto& t : threads) t.join();

    if (failed.load() || (cancel && cancel->load())) return nullptr;
    return bank;
}
