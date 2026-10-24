#pragma once

#include <algorithm>
#include <vector>

// Reverb estilo Freeverb (Jezar, dominio publico): 8 combs + 4 allpass por canal.
class Reverb {
public:
    void setSampleRate(float sr) {
        static const int kComb[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static const int kAllpass[4] = {556, 441, 341, 225};
        const float scale = sr / 44100.f;
        for (int ch = 0; ch < 2; ++ch) {
            const int spread = ch == 0 ? 0 : 23;
            for (int i = 0; i < 8; ++i) comb_[ch][i].init(static_cast<int>((kComb[i] + spread) * scale));
            for (int i = 0; i < 4; ++i) allpass_[ch][i].init(static_cast<int>((kAllpass[i] + spread) * scale));
        }
    }

    void clear() {
        for (auto& ch : comb_) for (auto& c : ch) c.clear();
        for (auto& ch : allpass_) for (auto& a : ch) a.clear();
    }

    // Soma o sinal molhado (wet) em outL/outR.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float wet) {
        const float feedback = 0.84f, damp = 0.25f, inputGain = 0.015f;
        for (int i = 0; i < n; ++i) {
            const float in = (inL[i] + inR[i]) * inputGain;
            float l = 0.f, r = 0.f;
            for (int c = 0; c < 8; ++c) {
                l += comb_[0][c].process(in, feedback, damp);
                r += comb_[1][c].process(in, feedback, damp);
            }
            for (int a = 0; a < 4; ++a) {
                l = allpass_[0][a].process(l);
                r = allpass_[1][a].process(r);
            }
            outL[i] += l * wet * 3.f;
            outR[i] += r * wet * 3.f;
        }
    }

private:
    struct Comb {
        std::vector<float> buf;
        int idx = 0;
        float store = 0.f;
        void init(int len) { buf.assign(len > 0 ? len : 1, 0.f); idx = 0; store = 0.f; }
        void clear() { std::fill(buf.begin(), buf.end(), 0.f); store = 0.f; }
        float process(float in, float feedback, float damp) {
            const float out = buf[idx];
            store = out * (1.f - damp) + store * damp + 1e-18f;  // evita denormais
            buf[idx] = in + store * feedback;
            if (++idx >= static_cast<int>(buf.size())) idx = 0;
            return out;
        }
    };
    struct Allpass {
        std::vector<float> buf;
        int idx = 0;
        void init(int len) { buf.assign(len > 0 ? len : 1, 0.f); idx = 0; }
        void clear() { std::fill(buf.begin(), buf.end(), 0.f); }
        float process(float in) {
            const float b = buf[idx];
            buf[idx] = in + b * 0.5f;
            if (++idx >= static_cast<int>(buf.size())) idx = 0;
            return b - in;
        }
    };

    Comb comb_[2][8];
    Allpass allpass_[2][4];
};
