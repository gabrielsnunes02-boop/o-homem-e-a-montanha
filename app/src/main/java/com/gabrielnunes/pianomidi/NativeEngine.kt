package com.gabrielnunes.pianomidi

import android.content.res.AssetManager
import android.media.midi.MidiDevice

/** Interface com o motor nativo (C++/Oboe/AMidi). */
object NativeEngine {
    init {
        System.loadLibrary("pianomidi")
    }

    external fun nativeCreate(assets: AssetManager): Boolean
    external fun nativeRestartAudio()
    external fun nativeLoadSamples(layers: Int)
    external fun nativeOpenMidiPort(device: MidiDevice, port: Int): Int
    external fun nativeCloseMidiPort(slot: Int)
    external fun nativeSendMidi(status: Int, d1: Int, d2: Int)
    external fun nativeSetParams(volume: Float, reverb: Float, velocityCurve: Float, transpose: Int, tuneHz: Float)
    external fun nativeGetStats(out: IntArray)
}

/** Fotografia do estado do motor, lida periodicamente pela UI. */
data class EngineStats(
    val sampleRate: Int = 0,
    val framesPerBurst: Int = 0,
    val bufferFrames: Int = 0,
    val xruns: Int = 0,
    val exclusive: Boolean = false,
    val mmap: Boolean = false,
    val latencyMs: Float = -1f,
    val lowLatency: Boolean = false,
    val voices: Int = 0,
    val lastNote: Int = -1,
    val lastVelocity: Int = 0,
    val midiEvents: Int = 0,
    val sustain: Boolean = false,
    val loadState: Int = 0,
    val loadProgress: Float = 0f,
    val bankMb: Int = 0,
) {
    val bufferMs: Float get() = if (sampleRate > 0) bufferFrames * 1000f / sampleRate else 0f

    companion object {
        private val raw = IntArray(16)

        fun read(): EngineStats {
            NativeEngine.nativeGetStats(raw)
            return EngineStats(
                sampleRate = raw[0], framesPerBurst = raw[1], bufferFrames = raw[2], xruns = raw[3],
                exclusive = raw[4] != 0, mmap = raw[5] != 0,
                latencyMs = if (raw[6] >= 0) raw[6] / 10f else -1f,
                lowLatency = raw[7] != 0, voices = raw[8], lastNote = raw[9], lastVelocity = raw[10],
                midiEvents = raw[11], sustain = raw[12] != 0, loadState = raw[13],
                loadProgress = raw[14] / 1000f, bankMb = raw[15],
            )
        }
    }
}
