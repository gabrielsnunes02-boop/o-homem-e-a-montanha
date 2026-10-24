# Piano MIDI (Android)

App de piano nativo para Android feito para tocar com teclado controlador MIDI
via cabo USB (OTG), com o menor atraso possível entre a tecla e o som.

## Instalar

1. No celular, abra a página de releases do repositório e baixe o
   `app-release.apk` da release **apk-latest** (atualizada a cada push).
2. Permita "instalar apps desconhecidos" para o navegador e instale.
3. Conecte o teclado com um adaptador OTG. O app detecta o teclado sozinho
   (e pode abrir automaticamente quando o teclado é conectado).

Requisitos: Android 10 ou mais novo (API nativa de MIDI).

## Por que a latência é baixa

| Etapa | O que o app faz |
|---|---|
| Entrada MIDI | API nativa **AMidi**, lida *dentro* do callback de áudio: sem troca de thread, sem Java/Kotlin no caminho da nota |
| Saída de áudio | **Oboe/AAudio** em modo `LowLatency` + `Exclusive` (MMAP quando o aparelho suporta), buffer de 2 bursts ajustado pelo `LatencyTuner` só se houver underrun |
| CPU | `PerformanceHint` (ADPF) pede clock maior quando a carga sobe (acordes, pedal) |
| Samples | o silêncio/pré-ataque do início de cada sample (3–17 ms) foi removido |
| Motor | C++ sem alocação nem locks na thread de áudio; flush-to-zero contra denormais |

A tela mostra a taxa, o tamanho do buffer, a API em uso, o intervalo real
entre callbacks ("bloco real"), a latência de saída estimada e os underruns.

### Ajustes de latência (seção "Latência")

- **Procurar menor latência**: testa AAudio e OpenSL ES com uso Game e Media
  (cada aparelho libera o caminho rápido para uma combinação diferente),
  mede o intervalo real entre callbacks e fica com a mais rápida. Roda
  sozinho na primeira abertura.
- **Buffer 1 bloco / 2 blocos**: 1 bloco é o mínimo possível; se houver
  underrun, o buffer cresce 1 bloco por vez automaticamente.
- **Modo leve**: interpolação linear e até 40 vozes (~4x menos CPU), para
  aparelhos fracos aguentarem o buffer mínimo sem estalar.

## Recursos

- Piano Salamander Grand (Yamaha C5), 4 ou 8 camadas de velocity, 30 notas
  amostradas (interpolação cúbica entre elas)
- Pedal de sustain (CC64), una corda (CC67), volume (CC7), all notes off
- Abafadores realistas (agudos a partir de F6 sem abafador)
- Reverb, curva de velocity (5 níveis), transposição ±12, afinação A4 415–466 Hz
- Continua tocando em segundo plano (serviço em primeiro plano), opcional

## Estrutura

```
app/src/main/cpp/engine/     motor portável (sampler, parser MIDI, reverb)
app/src/main/cpp/android/    Oboe + AMidi + JNI
app/src/main/java/...        UI (Jetpack Compose), conexão MIDI, serviço
app/src/main/assets/samples  samples Ogg Vorbis já processados
engine-tests/                testes do motor que rodam no PC
tools/prepare_samples.py     gera os samples a partir dos FLACs originais
```

## Compilar

- APK: `./gradlew assembleRelease` (requer Android SDK + NDK), ou deixe o
  GitHub Actions compilar (`.github/workflows/android.yml`).
- Testes do motor no PC:
  ```
  cmake -S engine-tests -B build-host && cmake --build build-host
  ./build-host/host_test app/src/main/assets demo.wav
  ```

O APK é assinado com `app/piano.keystore` (chave de desenvolvimento
versionada de propósito, para que cada build instale por cima do anterior).

## Créditos e licenças

- Samples: **Salamander Grand Piano V3** por Alexander Holm, licença
  [CC-BY 3.0](https://creativecommons.org/licenses/by/3.0/). Mapeamento de
  [sfzinstruments/SalamanderGrandPiano](https://github.com/sfzinstruments/SalamanderGrandPiano).
  Os samples foram cortados no início, encurtados na cauda e recodificados.
- [Oboe](https://github.com/google/oboe), Apache 2.0.
- [stb_vorbis](https://github.com/nothings/stb), domínio público / MIT.
- Reverb baseado no Freeverb (Jezar at Dreampoint), domínio público.
