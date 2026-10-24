package com.gabrielnunes.pianomidi

import android.app.Application
import android.content.Intent
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/** Mantem motor, MIDI e configuracoes vivos enquanto o processo existir. */
class PianoApplication : Application() {
    lateinit var midi: MidiConnector
        private set

    private val _settings = MutableStateFlow(Settings())
    val settings: StateFlow<Settings> = _settings

    var audioOk = false
        private set

    override fun onCreate() {
        super.onCreate()
        val s = Settings.load(this)
        _settings.value = s
        audioOk = NativeEngine.nativeCreate(assets)
        applyParams(s)
        NativeEngine.nativeLoadSamples(s.layers)
        midi = MidiConnector(this).also { it.start() }
    }

    fun update(transform: (Settings) -> Settings) {
        val old = _settings.value
        val new = transform(old)
        if (new == old) return
        _settings.value = new
        new.save(this)
        applyParams(new)
        if (new.layers != old.layers) NativeEngine.nativeLoadSamples(new.layers)
        if (new.background != old.background) {
            if (new.background) startPianoService() else stopService(Intent(this, PianoService::class.java))
        }
    }

    fun startPianoService() {
        if (_settings.value.background) startForegroundService(Intent(this, PianoService::class.java))
    }

    fun restartAudio() {
        NativeEngine.nativeRestartAudio()
    }

    private fun applyParams(s: Settings) {
        NativeEngine.nativeSetParams(s.volume, s.reverb, s.velocityCurve.exponent, s.transpose, s.tuneHz)
    }
}
