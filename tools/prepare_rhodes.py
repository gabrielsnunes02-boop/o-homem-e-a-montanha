#!/usr/bin/env python3
"""Prepara os samples do jRhodes3c (Rhodes Mark I Stage 73, 1977) para o app.

Le o SFZ estereo com loop, extrai os pontos de loop do bloco 'smpl' de cada
FLAC, remove o silencio inicial (ajustando o loop) e codifica em Ogg Vorbis.
Gera tambem 'regions.txt', o mapa lido pelo motor.

Uso:
  python3 tools/prepare_rhodes.py <pasta jRhodes3c-looped-flac-sfz> app/src/main/assets/rhodes

Samples: https://github.com/sfzinstruments/jlearman.jRhodes3c por Jeff Learman,
licenca CC BY-NC-SA 4.0 (uso nao comercial).
"""
import os
import re
import struct
import subprocess
import sys

import numpy as np

NOTE = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}


def key(name):
    m = re.fullmatch(r"([a-g])(#?)(-?\d)", name.strip().lower())
    if not m:
        return int(name)
    return NOTE[m[1]] + (1 if m[2] else 0) + 12 * (int(m[3]) + 1)  # c4 = 60


def parse_sfz(path):
    regions, group, cur = [], {}, None
    for line in open(path, encoding="latin-1"):
        line = line.split("//")[0].strip()
        if not line:
            continue
        if line == "<group>":
            group, cur = {}, None
            continue
        if line == "<region>":
            cur = dict(group)
            regions.append(cur)
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            (cur if cur is not None else group)[k.strip()] = v.strip()
    return regions


def flac_loop(path):
    """Le (inicio, fim) do loop no chunk RIFF 'smpl' guardado no FLAC."""
    d = open(path, "rb").read()
    i = 4
    while True:
        h = d[i]
        n = int.from_bytes(d[i + 1:i + 4], "big")
        body = d[i + 4:i + 4 + n]
        if (h & 0x7F) == 2 and body[:4] == b"riff" and b"smpl" in body:
            k = body.index(b"smpl")
            num_loops = struct.unpack_from("<I", body, k + 8 + 28)[0]
            if num_loops:
                start, end = struct.unpack_from("<II", body, k + 8 + 36 + 8)
                return start, end
        i += 4 + n
        if h & 0x80:
            return None


def load(path):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-ac", "2", "-f", "f32le", "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype=np.float32).reshape(-1, 2).copy()


def main():
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    regions = parse_sfz(os.path.join(src, "_jRhodes-stereo-looped.sfz"))
    lines, peaks, total = [], [], 0
    for r in regions:
        f = r["sample"]
        x = load(os.path.join(src, f))
        loop = flac_loop(os.path.join(src, f)) if r.get("loop_mode") == "loop_continuous" else None
        mono = np.abs(x).max(axis=1)
        onset = int(np.argmax(mono > max(mono.max() * 0.01, mono[:32].max() * 4)))
        cut = max(0, onset - 22)
        if loop and loop[0] <= cut:
            cut = 0
        x = x[cut:]
        fin = min(22, len(x))
        x[:fin] *= np.linspace(0, 1, fin, dtype=np.float32)[:, None]
        ls, le = (loop[0] - cut, loop[1] - cut) if loop else (0, 0)
        name = f.replace("-stereo.flac", ".ogg")
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "f32le", "-ar", "44100", "-ac", "2", "-i", "-",
                        "-c:a", "libvorbis", "-q:a", "7", os.path.join(dst, name)],
                       input=np.clip(x, -1, 1).tobytes(), check=True)
        peaks.append(float(mono.max()))
        total += len(x)
        lines.append(" ".join(str(v) for v in [
            name, key(r["lokey"]), key(r["hikey"]), r.get("lovel", "1"), r.get("hivel", "127"),
            key(r["pitch_keycenter"]), ls, le, r.get("ampeg_hold", "0"), r.get("ampeg_decay", "0"),
            r.get("ampeg_sustain", "100"), r.get("ampeg_release", "0.3")]))
    with open(os.path.join(dst, "regions.txt"), "w") as out:
        out.write("# arquivo lokey hikey lovel hivel raiz loop_ini loop_fim hold decay sustain% release\n")
        out.write("\n".join(lines) + "\n")
    print(f"{len(lines)} regioes, {total * 4 / 1e6:.1f} MB em RAM, pico max {20 * np.log10(max(peaks)):.1f} dB")


if __name__ == "__main__":
    main()
