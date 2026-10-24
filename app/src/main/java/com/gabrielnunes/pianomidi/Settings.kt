package com.gabrielnunes.pianomidi

import android.content.Context

enum class VelocityCurve(val label: String, val exponent: Float) {
    VERY_LIGHT("Muito leve", 0.5f),
    LIGHT("Leve", 0.75f),
    NORMAL("Normal", 1.0f),
    HEAVY("Pesado", 1.35f),
    VERY_HEAVY("Muito pesado", 1.8f),
}

enum class Instrument(val label: String) {
    PIANO("Piano de cauda"),
    RHODES("Rhodes"),
}

data class Settings(
    val instrument: Instrument = Instrument.PIANO,
    val volume: Float = 0.8f,
    val reverb: Float = 0.2f,
    val velocityCurve: VelocityCurve = VelocityCurve.NORMAL,
    val transpose: Int = 0,
    val tuneHz: Float = 440f,
    val layers: Int = 4,
    val background: Boolean = true,
    val audioRoute: Int = -1, // -1 = ainda nao buscou a menor latencia
    val bufferBursts: Int = 2,
    val lightMode: Boolean = false,
) {
    fun save(context: Context) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString("instrument", instrument.name)
            .putFloat("volume", volume)
            .putFloat("reverb", reverb)
            .putString("velocityCurve", velocityCurve.name)
            .putInt("transpose", transpose)
            .putFloat("tuneHz", tuneHz)
            .putInt("layers", layers)
            .putBoolean("background", background)
            .putInt("audioRoute", audioRoute)
            .putInt("bufferBursts", bufferBursts)
            .putBoolean("lightMode", lightMode)
            .apply()
    }

    companion object {
        private const val PREFS = "settings"

        fun load(context: Context): Settings {
            val p = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            val d = Settings()
            return Settings(
                instrument = runCatching { Instrument.valueOf(p.getString("instrument", null)!!) }
                    .getOrDefault(d.instrument),
                volume = p.getFloat("volume", d.volume),
                reverb = p.getFloat("reverb", d.reverb),
                velocityCurve = runCatching { VelocityCurve.valueOf(p.getString("velocityCurve", null)!!) }
                    .getOrDefault(d.velocityCurve),
                transpose = p.getInt("transpose", d.transpose),
                tuneHz = p.getFloat("tuneHz", d.tuneHz),
                layers = p.getInt("layers", d.layers),
                background = p.getBoolean("background", d.background),
                audioRoute = p.getInt("audioRoute", d.audioRoute),
                bufferBursts = p.getInt("bufferBursts", d.bufferBursts),
                lightMode = p.getBoolean("lightMode", d.lightMode),
            )
        }
    }
}

/** Combinacoes de API/uso de audio, na mesma ordem de kRoutes no C++. */
val AUDIO_ROUTES = listOf("AAudio · Game", "AAudio · Media", "OpenSL · Media", "OpenSL · Game")
