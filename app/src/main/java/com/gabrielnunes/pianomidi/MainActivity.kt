package com.gabrielnunes.pianomidi

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilterChip
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay
import java.util.Locale

class MainActivity : ComponentActivity() {
    private val app get() = application as PianoApplication

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1)
        }
        setContent {
            MaterialTheme(colorScheme = darkColorScheme(primary = Accent, secondary = Accent)) {
                PianoScreen(app)
            }
        }
    }

    override fun onStart() {
        super.onStart()
        app.startPianoService()
    }
}

private val Accent = Color(0xFFFFB300)
private val NOTE_NAMES = arrayOf("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")

private fun noteName(n: Int) = if (n < 0) "—" else "${NOTE_NAMES[n % 12]}${n / 12 - 1}"
private fun fmt(v: Float, d: Int = 1) = String.format(Locale("pt", "BR"), "%.${d}f", v)

@Composable
private fun PianoScreen(app: PianoApplication) {
    val settings by app.settings.collectAsState()
    val devices by app.midi.devices.collectAsState()
    var stats by remember { mutableStateOf(EngineStats()) }
    LaunchedEffect(Unit) {
        while (true) {
            stats = EngineStats.read()
            delay(150)
        }
    }

    Column(
        Modifier
            .fillMaxSize()
            .background(Color(0xFF121212))
            .systemBarsPadding()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Text("Piano MIDI", fontSize = 26.sp, fontWeight = FontWeight.Bold, color = Color.White)

        StatusCard(app, devices, stats)
        ActivityCard(stats)
        LatencyCard(app, settings, stats)

        Section("Som") {
            LabeledSlider("Volume", "${(settings.volume * 100).toInt()}%", settings.volume, 0f..1.5f) { v ->
                app.update { it.copy(volume = v) }
            }
            LabeledSlider("Reverb", "${(settings.reverb * 100).toInt()}%", settings.reverb, 0f..1f) { v ->
                app.update { it.copy(reverb = v) }
            }
        }

        Section("Resposta ao toque (curva de velocity)") {
            ChipRow(VelocityCurve.entries.map { it.label }, settings.velocityCurve.ordinal) { i ->
                app.update { it.copy(velocityCurve = VelocityCurve.entries[i]) }
            }
            Text(
                "Leve: menos força para tocar forte. Pesado: exige mais força.",
                color = Color.Gray, fontSize = 12.sp,
            )
        }

        Section("Transposição e afinação") {
            Stepper("Transpor", (if (settings.transpose > 0) "+" else "") + "${settings.transpose} st",
                onMinus = { app.update { it.copy(transpose = (it.transpose - 1).coerceAtLeast(-12)) } },
                onPlus = { app.update { it.copy(transpose = (it.transpose + 1).coerceAtMost(12)) } },
                onReset = { app.update { it.copy(transpose = 0) } })
            Stepper("Afinação (A4)", "${fmt(settings.tuneHz)} Hz",
                onMinus = { app.update { it.copy(tuneHz = (it.tuneHz - 0.5f).coerceAtLeast(415f)) } },
                onPlus = { app.update { it.copy(tuneHz = (it.tuneHz + 0.5f).coerceAtMost(466f)) } },
                onReset = { app.update { it.copy(tuneHz = 440f) } })
        }

        Section("Qualidade dos samples") {
            ChipRow(listOf("4 camadas (~170 MB)", "8 camadas (~340 MB)"), if (settings.layers >= 8) 1 else 0) { i ->
                app.update { it.copy(layers = if (i == 1) 8 else 4) }
            }
            Text(
                "Mais camadas = mais nuances de dinâmica, porém mais RAM. Em aparelhos de 4 GB, prefira 4.",
                color = Color.Gray, fontSize = 12.sp,
            )
        }

        Section("Sistema") {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Column(Modifier.weight(1f)) {
                    Text("Tocar em segundo plano", color = Color.White)
                    Text("Continua soando com outro app aberto (ex.: partitura).", color = Color.Gray, fontSize = 12.sp)
                }
                Switch(settings.background, { v -> app.update { it.copy(background = v) } })
            }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedButton({ playTestChord() }) { Text("Testar som") }
                OutlinedButton({ NativeEngine.nativeSendMidi(0xB0, 120, 0) }) { Text("Soltar notas") }
                OutlinedButton({ app.restartAudio(); app.midi.rescan() }) { Text("Reiniciar") }
            }
        }

        Text(
            "Som: Salamander Grand Piano V3 (Yamaha C5) por Alexander Holm, licença CC-BY 3.0.",
            color = Color.Gray, fontSize = 11.sp,
        )
        Spacer(Modifier.height(24.dp))
    }
}

private fun playTestChord() {
    Thread {
        val notes = intArrayOf(48, 60, 64, 67, 72)
        notes.forEach { NativeEngine.nativeSendMidi(0x90, it, 85); Thread.sleep(25) }
        Thread.sleep(1200)
        notes.forEach { NativeEngine.nativeSendMidi(0x80, it, 0) }
    }.start()
}

@Composable
private fun StatusCard(app: PianoApplication, devices: List<String>, s: EngineStats) {
    Section("Status") {
        val midiOk = devices.isNotEmpty()
        StatusLine(midiOk, if (midiOk) "Teclado: ${devices.joinToString()}" else "Conecte o teclado via cabo USB (OTG)")
        when (s.loadState) {
            1 -> {
                Text("Carregando samples… ${(s.loadProgress * 100).toInt()}%", color = Color.White)
                LinearProgressIndicator(progress = { s.loadProgress }, modifier = Modifier.fillMaxWidth())
            }
            2 -> StatusLine(true, "Samples carregados (${s.bankMb} MB)")
            3 -> StatusLine(false, "Erro ao carregar samples")
        }
        if (!app.audioOk && s.sampleRate == 0) {
            StatusLine(false, "Falha ao abrir a saída de áudio")
        } else if (s.probing) {
            Text("Procurando a menor latência…", color = Color.White)
            LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
        } else if (s.sampleRate > 0) {
            val fast = s.callbackGapMs in 0.1f..7f
            StatusLine(fast, buildString {
                append("Áudio ${s.sampleRate} Hz · buffer ${s.bufferFrames} (${fmt(s.bufferMs)} ms)")
                if (s.latencyMs > 0) append(" · saída ≈ ${fmt(s.latencyMs, 0)} ms")
            })
            Text(
                buildString {
                    append(if (s.aaudio) "AAudio" else "OpenSL ES")
                    if (s.callbackGapMs > 0) append(" · bloco real ${fmt(s.callbackGapMs)} ms")
                    append(if (s.mmap) " · MMAP" else "")
                    if (s.xruns >= 0) append(" · underruns ${s.xruns}")
                },
                color = Color.Gray, fontSize = 12.sp,
            )
        }
    }
}

@Composable
private fun LatencyCard(app: PianoApplication, settings: Settings, s: EngineStats) {
    val results by app.probeResults.collectAsState()
    Section("Latência") {
        if (app.recoveredFromCrash) {
            Text(
                "A última combinação de áudio fechou o app; voltei para a padrão.",
                color = Color(0xFFE57373), fontSize = 13.sp,
            )
        }
        Button({ app.findLowestLatency() }, enabled = !s.probing, modifier = Modifier.fillMaxWidth()) {
            Text(if (s.probing) "Testando…" else "Procurar menor latência")
        }
        Text(
            "Testa as 4 combinações de áudio do Android e fica com a mais rápida (fica mudo por ~2 s).",
            color = Color.Gray, fontSize = 12.sp,
        )
        Text("Combinação de áudio", color = Color.White)
        ChipRow(
            AUDIO_ROUTES.mapIndexed { i, name ->
                val r = results.getOrNull(i)
                when {
                    results.isEmpty() -> name
                    r == null -> "$name · falhou"
                    else -> "$name · ${fmt(r)} ms"
                }
            },
            settings.audioRoute.coerceAtLeast(0),
            columns = 1,
        ) { i -> if (!s.probing) app.update { it.copy(audioRoute = i) } }

        Text("Buffer", color = Color.White)
        ChipRow(listOf("1 bloco (mínimo)", "2 blocos (seguro)"), if (settings.bufferBursts <= 1) 0 else 1, columns = 2) { i ->
            if (!s.probing) app.update { it.copy(bufferBursts = i + 1) }
        }
        Text(
            "Com 1 bloco a latência cai, mas pode estalar em aparelhos fracos. Se estalar, o app aumenta o buffer sozinho.",
            color = Color.Gray, fontSize = 12.sp,
        )

        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text("Modo leve (CPU fraca)", color = Color.White)
                Text("Interpolação linear e até 40 vozes: usa ~4x menos CPU.", color = Color.Gray, fontSize = 12.sp)
            }
            Switch(settings.lightMode, { v -> app.update { it.copy(lightMode = v) } })
        }

        val d = app.deviceAudio
        Text(
            "Aparelho: ${d.nativeSampleRate} Hz, bloco nativo ${d.nativeBurst} frames" +
                " · baixa latência: ${if (d.lowLatency) "sim" else "não"}" +
                " · pro: ${if (d.pro) "sim" else "não"}",
            color = Color.Gray, fontSize = 12.sp,
        )
    }
}

@Composable
private fun ActivityCard(s: EngineStats) {
    Section("Tocando") {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text(noteName(s.lastNote), fontSize = 34.sp, fontWeight = FontWeight.Bold, color = Accent)
                Text("velocity ${s.lastVelocity}", color = Color.Gray, fontSize = 12.sp)
            }
            Column(horizontalAlignment = Alignment.End) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Pedal ", color = Color.White)
                    Box(Modifier.size(14.dp).background(if (s.sustain) Accent else Color.DarkGray, CircleShape))
                }
                Text("${s.voices} vozes", color = Color.Gray, fontSize = 12.sp)
                Text("${s.midiEvents} eventos MIDI", color = Color.Gray, fontSize = 12.sp)
            }
        }
    }
}

@Composable
private fun Section(title: String, content: @Composable () -> Unit) {
    Card(
        colors = CardDefaults.cardColors(containerColor = Color(0xFF1E1E1E)),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text(title, color = Accent, fontWeight = FontWeight.SemiBold)
            content()
        }
    }
}

@Composable
private fun StatusLine(ok: Boolean, text: String) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(Modifier.size(10.dp).background(if (ok) Color(0xFF4CAF50) else Color(0xFFE57373), CircleShape))
        Spacer(Modifier.width(8.dp))
        Text(text, color = Color.White, fontSize = 14.sp)
    }
}

@Composable
private fun LabeledSlider(
    label: String, value: String, current: Float, range: ClosedFloatingPointRange<Float>,
    onChange: (Float) -> Unit,
) {
    Column {
        Row {
            Text(label, color = Color.White, modifier = Modifier.weight(1f))
            Text(value, color = Color.Gray)
        }
        Slider(current, onChange, valueRange = range)
    }
}

@Composable
private fun ChipRow(labels: List<String>, selected: Int, columns: Int = 3, onSelect: (Int) -> Unit) {
    Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
        labels.chunked(columns).forEachIndexed { row, chunk ->
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                chunk.forEachIndexed { col, label ->
                    val i = row * columns + col
                    FilterChip(selected = i == selected, onClick = { onSelect(i) }, label = { Text(label) })
                }
            }
        }
    }
}

@Composable
private fun Stepper(label: String, value: String, onMinus: () -> Unit, onPlus: () -> Unit, onReset: () -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, color = Color.White, modifier = Modifier.weight(1f))
        Button(onMinus, Modifier.size(44.dp), contentPadding = androidx.compose.foundation.layout.PaddingValues(0.dp)) {
            Text("−", fontSize = 20.sp)
        }
        Text(
            value, color = Color.White, modifier = Modifier.width(96.dp).padding(horizontal = 4.dp),
            textAlign = androidx.compose.ui.text.style.TextAlign.Center,
        )
        Button(onPlus, Modifier.size(44.dp), contentPadding = androidx.compose.foundation.layout.PaddingValues(0.dp)) {
            Text("+", fontSize = 20.sp)
        }
        Spacer(Modifier.width(6.dp))
        OutlinedButton(onReset, contentPadding = androidx.compose.foundation.layout.PaddingValues(horizontal = 10.dp)) {
            Text("↺")
        }
    }
}
