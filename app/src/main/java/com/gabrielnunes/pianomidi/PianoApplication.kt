package com.gabrielnunes.pianomidi

import android.app.Application
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/** Caracteristicas de audio declaradas pelo aparelho. */
data class DeviceAudioInfo(
    val nativeSampleRate: Int,
    val nativeBurst: Int,
    val lowLatency: Boolean,
    val pro: Boolean,
)

/** Mantem motor, MIDI e configuracoes vivos enquanto o processo existir. */
class PianoApplication : Application() {
    lateinit var midi: MidiConnector
        private set
    lateinit var deviceAudio: DeviceAudioInfo
        private set

    private val _settings = MutableStateFlow(Settings())
    val settings: StateFlow<Settings> = _settings

    /** Intervalo medido (ms) de cada rota na ultima busca; null = nao medida. */
    private val _probeResults = MutableStateFlow<List<Float?>>(emptyList())
    val probeResults: StateFlow<List<Float?>> = _probeResults

    var audioOk = false
        private set

    override fun onCreate() {
        super.onCreate()
        val s = Settings.load(this)
        _settings.value = s
        deviceAudio = readDeviceAudio()
        audioOk = NativeEngine.nativeCreate(
            assets, deviceAudio.nativeSampleRate, deviceAudio.nativeBurst,
            s.audioRoute.coerceAtLeast(0), s.bufferBursts,
        )
        NativeEngine.nativeSetLightMode(s.lightMode)
        applyParams(s)
        NativeEngine.nativeLoadSamples(s.layers)
        midi = MidiConnector(this).also { it.start() }
        if (s.audioRoute < 0) findLowestLatency()
    }

    fun update(transform: (Settings) -> Settings) {
        val old = _settings.value
        val new = transform(old)
        if (new == old) return
        _settings.value = new
        new.save(this)
        applyParams(new)
        if (new.layers != old.layers) NativeEngine.nativeLoadSamples(new.layers)
        if (new.lightMode != old.lightMode) NativeEngine.nativeSetLightMode(new.lightMode)
        if (new.audioRoute != old.audioRoute || new.bufferBursts != old.bufferBursts) restartAudio()
        if (new.background != old.background) {
            if (new.background) startPianoService() else stopService(Intent(this, PianoService::class.java))
        }
    }

    /** Testa as combinacoes de API de audio e fica com a de menor latencia real. */
    fun findLowestLatency() {
        Thread {
            val r = NativeEngine.nativeProbeAudio()
            _probeResults.value = (1 until r.size).map { if (r[it] >= 0) r[it] / 10f else null }
            if (r[0] >= 0) {
                val s = _settings.value.copy(audioRoute = r[0])
                _settings.value = s
                s.save(this)
            }
        }.start()
    }

    fun startPianoService() {
        if (_settings.value.background) startForegroundService(Intent(this, PianoService::class.java))
    }

    fun restartAudio() {
        val s = _settings.value
        NativeEngine.nativeConfigureAudio(s.audioRoute.coerceAtLeast(0), s.bufferBursts)
    }

    private fun applyParams(s: Settings) {
        NativeEngine.nativeSetParams(s.volume, s.reverb, s.velocityCurve.exponent, s.transpose, s.tuneHz)
    }

    private fun readDeviceAudio(): DeviceAudioInfo {
        val am = getSystemService(AudioManager::class.java)
        return DeviceAudioInfo(
            nativeSampleRate = am.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)?.toIntOrNull() ?: 0,
            nativeBurst = am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER)?.toIntOrNull() ?: 0,
            lowLatency = packageManager.hasSystemFeature(PackageManager.FEATURE_AUDIO_LOW_LATENCY),
            pro = packageManager.hasSystemFeature(PackageManager.FEATURE_AUDIO_PRO),
        )
    }
}
