# DistortX — Distortion Pedal Plugin

DistortX es un plugin de audio digital (VST3, AU, Standalone) desarrollado en **C++17** con el framework **JUCE**. Emula la saturación armónica y la calidez de un pedal de distorsión analógico, optimizado para uso en vivo y producción con **Reaper** (y compatible con cualquier DAW VST3/AU).

## Características Principales

*   **Noise Gate integrado:** Compuerta de ruido al inicio de la cadena DSP para eliminar hum y ruido de fondo antes de la etapa de ganancia.
*   **Soft & Hard Clipping:** Algoritmos de waveshaping dedicados:
    *   **Soft (Overdrive):** `tanh(x)` — armónicos impares, calidez valvular.
    *   **Hard (Distortion/Fuzz):** `clamp(-1, 1, x)` — recorte abrupto, agresivo.
*   **Cadena DSP:** `Noise Gate → Pre-HPF (120Hz) → Drive → Waveshaper+Oversampling → Tone (LPF) → Level`
*   **Controles:**
    *   **Gate Threshold / Gate Decay:** Control del noise gate.
    *   **Drive:** Ganancia de entrada (0–36 dB) hacia la etapa de saturación.
    *   **Tone:** Filtro paso-bajo (800 Hz – 18 kHz) para esculpir el color post-distorsión.
    *   **Level:** Ganancia de salida (-24 a +12 dB).
    *   **Mix:** Mezcla Dry/Wet para procesamiento paralelo.
    *   **Bypass:** Bypass limpio sin chasquidos.
    *   **Saturation Type:** Selector Soft / Hard.
*   **Oversampling x4:** Anti-aliasing interno (`juce::dsp::Oversampling`) para eliminar artefactos inarmónicos.
*   **Parameter Smoothing:** `juce::SmoothedValue` + rampas de 20 ms — sin zipper noise al automatizar en Reaper.
*   **Lock-free DSP:** Sin `new`, mutex ni bloqueos dentro de `processBlock()`.

## Stack Tecnológico

*   **Lenguaje:** C++17
*   **Framework:** JUCE 7.0.9
*   **Build System:** CMake 3.20+
*   **Formatos:** VST3 · AU · Standalone
*   **DAW principal:** Reaper (compatibilidad general VST3/AU)

## Compilación

Requiere **CMake 3.20+** y MSVC (Windows) o Apple Clang (macOS). JUCE se descarga automáticamente vía `FetchContent`.

```bash
# 1. Clona el repositorio
git clone https://github.com/Jorgez-tech/distortion_pedal.git
cd distortion_pedal

# 2. Configura (descarga JUCE automáticamente)
cmake -B build

# 3. Compila en Release
cmake --build build --config Release
```

El artefacto `.vst3` se genera en `build/DistortX_artefacts/Release/VST3/`.

## Arquitectura

Separación estricta de responsabilidades siguiendo el modelo JUCE lock-free:

| Componente | Responsabilidad |
|-----------|----------------|
| `PluginProcessor` | DSP en hilo de audio — sin locks, sin allocations |
| `PluginEditor` | UI en hilo principal |
| `APVTS` | Estado, persistencia y comunicación entre UI ↔ DSP ↔ Host |
| `OversampledClipper` | Encapsula Oversampling x4 + WaveShaper |

## Licencia

*Proyecto en desarrollo.*
