#pragma once

#include <cstddef>
#include <cstdint>

// Parser de fluxo de bytes MIDI 1.0 (suporta running status, ignora SysEx e
// mensagens de tempo real como clock/active sensing).
class MidiParser {
public:
    template <typename F>
    void feed(const uint8_t* bytes, size_t n, F&& emit) {
        for (size_t i = 0; i < n; ++i) {
            const uint8_t b = bytes[i];
            if (b >= 0xF8) continue;  // tempo real: nao altera o running status
            if (b == 0xF0) { inSysex_ = true; status_ = 0; continue; }
            if (b == 0xF7) { inSysex_ = false; continue; }
            if (b & 0x80) {
                inSysex_ = false;
                status_ = b;
                count_ = 0;
                needed_ = dataBytesFor(b);
                if (needed_ == 0) status_ = 0;
                continue;
            }
            if (inSysex_ || status_ == 0) continue;
            data_[count_++] = b;
            if (count_ == needed_) {
                if (status_ < 0xF0) emit(status_, data_[0], needed_ > 1 ? data_[1] : uint8_t{0});
                else status_ = 0;  // system common nao tem running status
                count_ = 0;
            }
        }
    }

    void reset() { status_ = 0; count_ = 0; needed_ = 0; inSysex_ = false; }

private:
    static int dataBytesFor(uint8_t status) {
        switch (status & 0xF0) {
            case 0xC0: case 0xD0: return 1;
            case 0xF0:
                if (status == 0xF2) return 2;
                if (status == 0xF1 || status == 0xF3) return 1;
                return 0;
            default: return 2;
        }
    }

    uint8_t status_ = 0;
    uint8_t data_[2] = {0, 0};
    int count_ = 0;
    int needed_ = 0;
    bool inSysex_ = false;
};
