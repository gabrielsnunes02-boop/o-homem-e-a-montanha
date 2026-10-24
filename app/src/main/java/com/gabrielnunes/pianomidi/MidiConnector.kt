package com.gabrielnunes.pianomidi

import android.content.Context
import android.media.midi.MidiDevice
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/**
 * Conecta automaticamente qualquer dispositivo MIDI com portas de saida
 * (teclado USB) e entrega cada porta ao motor nativo, que le os eventos
 * direto na thread de audio.
 */
class MidiConnector(context: Context) {
    private val manager = context.getSystemService(MidiManager::class.java)
    private val handler = Handler(Looper.getMainLooper())
    private val opened = mutableMapOf<Int, Opened>()
    private val pending = mutableSetOf<Int>()
    private val _devices = MutableStateFlow<List<String>>(emptyList())
    val devices: StateFlow<List<String>> = _devices

    private class Opened(val device: MidiDevice, val slots: List<Int>, val name: String)

    private val callback = object : MidiManager.DeviceCallback() {
        override fun onDeviceAdded(info: MidiDeviceInfo) = connect(info)
        override fun onDeviceRemoved(info: MidiDeviceInfo) = disconnect(info.id)
    }

    fun start() {
        if (manager == null) return
        if (Build.VERSION.SDK_INT >= 33) {
            manager.registerDeviceCallback(
                MidiManager.TRANSPORT_MIDI_BYTE_STREAM, { handler.post(it) }, callback,
            )
            manager.getDevicesForTransport(MidiManager.TRANSPORT_MIDI_BYTE_STREAM).forEach(::connect)
        } else {
            @Suppress("DEPRECATION")
            manager.registerDeviceCallback(callback, handler)
            @Suppress("DEPRECATION")
            manager.devices.forEach(::connect)
        }
    }

    /** Reconecta tudo (por exemplo, se o teclado parou de responder). */
    fun rescan() {
        opened.keys.toList().forEach(::disconnect)
        start()
    }

    private fun connect(info: MidiDeviceInfo) {
        if (info.outputPortCount == 0 || info.id in opened || info.id in pending) return
        if (info.type == MidiDeviceInfo.TYPE_BLUETOOTH) return // por enquanto, so USB/virtual
        pending += info.id
        manager?.openDevice(info, { device ->
            pending -= info.id
            if (device == null) {
                Log.w(TAG, "nao foi possivel abrir ${nameOf(info)}")
                return@openDevice
            }
            val slots = (0 until info.outputPortCount)
                .map { NativeEngine.nativeOpenMidiPort(device, it) }
                .filter { it >= 0 }
            if (slots.isEmpty()) {
                device.close()
                return@openDevice
            }
            opened[info.id] = Opened(device, slots, nameOf(info))
            publish()
        }, handler)
    }

    private fun disconnect(id: Int) {
        val o = opened.remove(id) ?: return
        o.slots.forEach(NativeEngine::nativeCloseMidiPort)
        runCatching { o.device.close() }
        publish()
    }

    private fun publish() {
        _devices.value = opened.values.map { it.name }
    }

    private fun nameOf(info: MidiDeviceInfo): String {
        val p = info.properties
        return p.getString(MidiDeviceInfo.PROPERTY_NAME)
            ?: listOfNotNull(
                p.getString(MidiDeviceInfo.PROPERTY_MANUFACTURER),
                p.getString(MidiDeviceInfo.PROPERTY_PRODUCT),
            ).joinToString(" ").ifBlank { "Dispositivo MIDI" }
    }

    companion object {
        private const val TAG = "PianoMidi"
    }
}
