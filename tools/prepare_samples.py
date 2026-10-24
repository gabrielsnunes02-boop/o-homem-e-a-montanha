#!/usr/bin/env python3
"""Prepara os samples do Salamander Grand Piano V3 para o app.

Para cada nota/camada:
  * remove o silencio/pre-ataque do inicio (reduz a latencia percebida);
  * limita o comprimento da cauda (economia de RAM) com fade-out suave;
  * codifica em Ogg Vorbis estereo 48 kHz.

Uso:
  python3 tools/prepare_samples.py <pasta_com_flacs> app/src/main/assets/samples

Os FLACs originais vem de
https://github.com/sfzinstruments/SalamanderGrandPiano (pasta Samples/),
licenca CC-BY 3.0, autor Alexander Holm.
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

import numpy as np

SR = 48000
NOTES = ["A0", "C1", "D#1", "F#1", "A1", "C2", "D#2", "F#2", "A2", "C3",
         "D#3", "F#3", "A3", "C4", "D#4", "F#4", "A4", "C5", "D#5", "F#5",
         "A5", "C6", "D#6", "F#6", "A6", "C7", "D#7", "F#7", "A7", "C8"]
LAYERS = [1, 4, 6, 8, 10, 12, 14, 16]
FADE_S = 1.5


def max_len_s(region):
    if region <= 8:    # A0..A2
        return 10.0
    if region <= 16:   # C3..A4
        return 8.0
    return 6.0         # C5 em diante (agudos ja decaem rapido)


def load(path):
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", path, "-ar", str(SR), "-ac", "2",
         "-f", "f32le", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype=np.float32).reshape(-1, 2).copy()


def process(x, region):
    mono = np.abs(x).max(axis=1)
    peak = float(mono.max())
    noise = float(mono[:64].max())
    thr = max(peak * 0.01, noise * 4.0)
    onset = int(np.argmax(mono > thr))
    start = max(0, onset - 24)  # 0,5 ms de margem antes do ataque
    x = x[start:]

    # Fim natural: ultimo bloco de 50 ms acima de -60 dB do pico.
    w = 2400
    n = len(x) // w
    env = np.sqrt((x[:n * w] ** 2).mean(axis=1).reshape(n, w).mean(axis=1))
    above = np.where(env > env.max() * 1e-3)[0]
    natural_end = (above[-1] + 1) * w if len(above) else len(x)
    end = min(len(x), natural_end, int(max_len_s(region) * SR))
    x = x[:end]

    fade = min(int(FADE_S * SR), len(x) // 2)
    t = np.linspace(0.0, 1.0, fade, dtype=np.float32)
    x[-fade:] *= (0.5 + 0.5 * np.cos(np.pi * t))[:, None]
    fin = min(24, len(x))
    x[:fin] *= np.linspace(0.0, 1.0, fin, dtype=np.float32)[:, None]
    return x, start


def encode(x, out):
    pcm = np.clip(x, -1.0, 1.0).astype(np.float32).tobytes()
    subprocess.run(
        ["ffmpeg", "-v", "error", "-y", "-f", "f32le", "-ar", str(SR),
         "-ac", "2", "-i", "-", "-c:a", "libvorbis", "-q:a", "5", out],
        input=pcm, check=True)


def job(args):
    src, dst, region, note, layer = args
    x = load(os.path.join(src, f"{note}v{layer}.flac"))
    y, cut = process(x, region)
    name = f"{note.replace('#', 's')}v{layer}.ogg"
    encode(y, os.path.join(dst, name))
    return name, cut, len(y)


def main():
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    jobs = [(src, dst, r, n, l) for l in LAYERS for r, n in enumerate(NOTES)]
    total = {l: 0 for l in LAYERS}
    with ThreadPoolExecutor(os.cpu_count()) as ex:
        for (name, cut, frames), j in zip(ex.map(job, jobs), jobs):
            total[j[4]] += frames
            print(f"{name:10s} corte={cut / SR * 1000:5.1f} ms dur={frames / SR:5.2f} s")
    for l, f in total.items():
        print(f"camada v{l}: {f * 4 / 1e6:.1f} MB em RAM (int16 estereo)")


if __name__ == "__main__":
    main()
