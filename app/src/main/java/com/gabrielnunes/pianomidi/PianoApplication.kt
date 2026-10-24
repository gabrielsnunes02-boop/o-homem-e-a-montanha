package com.gabrielnunes.pianomidi

import android.app.Application
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Handler
import android.os.Looper
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

    /** true se a ultima abertura caiu com uma combinacao de audio e o app voltou para a padrao. */
    var recoveredFromCrash = false
        private set

    private val handler = Handler(Looper.getMainLooper())
    private val guardPrefs by lazy { getSharedPreferences("audio_guard", MODE_PRIVATE) }
    private val confirmRoute = Runnable { guardPrefs.edit().putBoolean("pending", false).commit() }

    override fun onCreate() {
        super.onCreate()
        var s = Settings.load(this)
        if (guardPrefs.getBoolean("pending", false)) {
            // O processo morreu enquanto testava uma combinacao: volta para a padrao.
            recoveredFromCrash = true
            s = s.copy(audioRoute = 0, bufferBursts = 2)
            s.save(this)
            guardPrefs.edit().putBoolean("pending", false).commit()
        }
        _settings.value = s
        _probeResults.value = guardPrefs.getString("probe", "").orEmpty().split(',')
            .filter { it.isNotEmpty() }.map { it.toFloatOrNull() }
        deviceAudio = readDeviceAudio()
        armGuard()
        audioOk = NativeEngine.nativeCreate(
            assets, deviceAudio.nativeSampleRate, deviceAudio.nativeBurst,
            s.audioRoute.coerceAtLeast(0), s.bufferBursts,
        )
        NativeEngine.nativeSetLightMode(s.lightMode)
        applyParams(s)
        NativeEngine.nativeLoadInstrument(s.instrument.ordinal, s.layers)
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
        if (new.instrument != old.instrument ||
            (new.instrument == Instrument.PIANO && new.layers != old.layers)
        ) {
            NativeEngine.nativeLoadInstrument(new.instrument.ordinal, new.layers)
        }
        if (new.lightMode != old.lightMode) NativeEngine.nativeSetLightMode(new.lightMode)
        if (new.audioRoute != old.audioRoute || new.bufferBursts != old.bufferBursts) restartAudio()
        if (new.background != old.background) {
            if (new.background) startPianoService() else stopService(Intent(this, PianoService::class.java))
        }
    }

    /** Testa as combinacoes de API de audio e fica com a de menor latencia real. */
    fun findLowestLatency() {
        armGuard()
        Thread {
            val r = NativeEngine.nativeProbeAudio()
            val results = (1 until r.size).map { if (r[it] >= 0) r[it] / 10f else null }
            _probeResults.value = results
            guardPrefs.edit().putString("probe", results.joinToString(",") { it?.toString() ?: "x" }).commit()
            handler.post { armGuard() }
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
        armGuard()
        val s = _settings.value
        NativeEngine.nativeConfigureAudio(s.audioRoute.coerceAtLeast(0), s.bufferBursts)
    }

    /**
     * Marca a configuracao de audio atual como "em teste" (gravado na hora) e so
     * confirma depois de 4 s rodando. Se o app cair antes, a proxima abertura
     * volta para a combinacao padrao em vez de ficar travada.
     */
    private fun armGuard() {
        guardPrefs.edit().putBoolean("pending", true).commit()
        handler.removeCallbacks(confirmRoute)
        handler.postDelayed(confirmRoute, 4000)
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
