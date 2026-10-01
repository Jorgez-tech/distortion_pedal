# Plan de Proyecto: Plugin de Pedal de Distorsión "DistortX"

## 1. Resumen Ejecutivo y Alcance
**DistortX** es un plugin de audio digital profesional (formatos VST3, AU y Standalone) desarrollado en **C++17** con el framework **JUCE 7**. Su objetivo principal es emular el comportamiento dinámico, la calidez armónica y el grano sonoro de un pedal de distorsión analógico clásico (estilo stompbox), ofreciendo desde saturación ligera (*overdrive*) hasta distorsión pesada (*hard clipping / fuzz*).

El plugin está optimizado para flujos de trabajo en directo y estudio con el DAW **Reaper** utilizando interfaces de audio como la **Behringer UMC22** (latencia ultra-baja, sample rates de 44.1 kHz a 96 kHz y buffers de 64 a 256 samples).

---

## 2. Stack Tecnológico y Entorno de Desarrollo
* **Lenguaje:** C++17
* **Framework:** JUCE 7.0.9 (módulos: `juce_audio_processors`, `juce_audio_utils`, `juce_dsp`, `juce_gui_basics`, `juce_gui_extra`)
* **Build System:** CMake 3.20+
* **Integración Continua (CI/CD):** GitHub Actions con compilación cruzada para Windows (MSVC) y macOS (Apple Clang)
* **DAW de Referencia:** Cockos Reaper v6/v7 (ASIO / CoreAudio)
* **Control de Versiones:** Git & GitHub (`Jorgez-tech/distortion_pedal`)

---

## 3. Especificación de Requisitos

### 3.1 Requisitos Funcionales (DSP & Audio)
1. **Noise Gate Integrado:** Atenuación de siseo y ruido residual previo a la etapa de alta ganancia (Threshold: -100 dB a 0 dB, Decay: 5 ms a 500 ms).
2. **Pre-Filtrado HPF (Tight Low-End):** Filtro pasa-altos fijo a 120 Hz para eliminar frecuencias subsónicas y prevenir saturación fangosa (*muddy bass*).
3. **Control Drive:** Rango de 0 dB a +36 dB con curva logarítmica para control progresivo de saturación.
4. **Etapa de Saturación Dual (WaveShaper):**
   * **Soft Clipping:** $f(x) = \tanh(x)$ para emulación de saturación valvular con armónicos impares suaves.
   * **Hard Clipping:** $f(x) = \text{clamp}(x, -1, 1)$ para distorsión agresiva y corte abrupto de onda.
5. **Anti-Aliasing Interno (Oversampling x4):** Sobre-muestreo por polifase IIR de 2 etapas ($2^2 = 4\times$) para aislar armónicos superiores por encima de Nyquist.
6. **Compensación de Latencia y Fase (PDC & Phase Alignment):**
   * Reporte de latencia de oversampling al DAW mediante `setLatencySamples()`.
   * Compensación temporal del `dryBuffer` para evitar filtrado en peine (*comb filtering*) en la mezcla Dry/Wet.
7. **Control Tone (Post-LPF):** Filtro pasa-bajos biquad o State-Variable (800 Hz a 18 kHz) para esculpir agudos.
8. **Control Level & Mix:** Volumen de salida (-24 dB a +12 dB) y balance Dry/Wet procesado con vectorización SIMD (`FloatVectorOperations`).
9. **Soft-Bypass (De-clicking):** Transición suave de 10 ms para evitar clics al activar o desactivar el efecto.

### 3.2 Requisitos No Funcionales y Reglas de Tiempo Real
* **Zero-Allocation en Audio Thread:** Prohibido el uso de `new`, `malloc`, `std::make_unique` o llamadas a métodos de JUCE que reserven memoria en `processBlock()` (incluyendo regeneración de `IIR::Coefficients` dinámicos).
* **Lock-Free / Non-blocking:** Sin primitivas de sincronización bloqueantes (`std::mutex`, `std::lock_guard`, semáforos) en el hilo de audio.
* **Anti-Zipper Noise:** Suavizado continuo de parámetros continuos mediante `juce::SmoothedValue` y rampas de 20 ms.
* **Consumo de CPU:** Menor al 1.5% de CPU en un hilo único a 48 kHz / 128 samples.

---

## 4. Arquitectura del Software

```
                  ┌─────────────────────────────────────────────────────────┐
                  │                 DistortXAudioProcessor                  │
                  │                   (Hilo de Audio DSP)                   │
                  └────────────────────────────┬────────────────────────────┘
                                               │ APVTS (Lock-free atomics)
                                               ▼
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│ CADENA DE PROCESAMIENTO DE AUDIO (DSP CHAIN)                                                    │
│                                                                                                 │
│  Input ──► [Noise Gate] ──┬──► [Pre-HPF 120Hz] ──► [Drive Gain] ──► [x4 Oversampling Clipper]   │
│                           │                                                   │                 │
│                           │                                         [Post-LPF Tone]             │
│                           │                                                   │                 │
│                           │                                          [Output Level Gain]        │
│                           │                                                   │                 │
│                           ▼                                                   ▼                 │
│                    [Delay Match] ────────────────────────────────────► [SIMD Dry/Wet Mix]       │
│                                                                               │                 │
│                                                                               ▼                 │
│  Output ◄─────────────────────────────────────────────────────────── [Soft-Bypass Gate]        │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
                                               ▲
                                               │ Attachments
                  ┌────────────────────────────┴────────────────────────────┐
                  │               DistortXAudioProcessorEditor              │
                  │                  (Hilo Principal - GUI)                 │
                  └─────────────────────────────────────────────────────────┘
```

---

## 5. Especificación de la Interfaz de Usuario (UI / LookAndFeel)
* **Concepto Visual:** Chasis metálico de pedal stompbox analógico de alta gama (estilo *matte charcoal/black metal*, serigrafía blanca/dorada, tornillos perimetrales y textura sutil).
* **Controles:**
  * 6 Perillas rotatorias personalizadas (`DistortXRotaryKnob`) con línea indicadora de alto contraste y puntero visual.
  * Etiquetas de texto descriptivas bajo cada perilla (`GATE`, `DECAY`, `DRIVE`, `TONE`, `LEVEL`, `MIX`).
  * Indicador LED joya luminoso (brillante cuando está encendido, apagado en Bypass).
  * Selector basculante/toggle para tipo de saturación (`SOFT` / `HARD`).
  * Interruptor de pie (*footswitch*) o botón iluminado de `BYPASS`.
* **Escalabilidad y Layout:** Dimensiones base de 740×320 px con márgenes simétricos y tipografía moderna anti-aliased.

---

## 6. Cronograma de Hitos y Estado Actual

| Hito | Descripción | Estado |
| :--- | :--- | :---: |
| **Hito 1** | Infraestructura CMake, JUCE 7.0.9, CI en GitHub Actions | ✅ Completado |
| **Hito 2** | Cadena DSP base (Gate, HPF, Drive, Oversampling x4, Waveshaper, LPF, Mix SIMD) | ✅ Completado |
| **Hito 3** | APVTS: 8 Parámetros, persistencia XML y atomics lock-free | ✅ Completado |
| **Hito 4** | **Saneamiento & Auditoría DSP:** Zero-allocation en biquads, funciones libres noexcept, reporte PDC y retardo Dry | ✅ Completado (v0.3.1) |
| **Hito 5** | **Interfaz Gráfica (Fase 4):** `DistortXLookAndFeel`, perillas stompbox analógicas, LED joya reactivo, tornillos y serigrafía | ✅ Completado (v0.3.1) |
| **Hito 6** | **Validación & QA (Fase 5):** Tests unitarios Catch2 (curvas, THD, estabilidad de filtros, latencia, APVTS) — 100% pasando | ✅ Completado (v0.4.0) |
| **Hito 7** | **Presets & Release (Fase 6):** Presets de fábrica (*Warm Crunch, Tight Lead, Vintage Blues, Wall of Sound, Clean Boost, Parallel Aggression*), selector GUI y release v1.0.0 | ✅ Completado (v1.0.0) |

---

## 7. Matriz de Riesgos Técnicos y Mitigaciones

| Riesgo | Impacto | Mitigación Técnica |
| :--- | :---: | :--- |
| **Heap Allocation en `makeLowPass`** | Crítico (Audio Dropouts) | Calcular coeficientes biquad con fórmulas cerradas en memoria estática o migrar a `StateVariableTPTFilter`. |
| **Desfase en Dry/Wet (Comb Filtering)** | Moderado (Pérdida de tono) | Insertar una línea de retardo de compensación en `dryBuffer` equivalente a `oversampling->getLatencyInSamples()`. |
| **Latencia no reportada a Reaper** | Moderado (Desfase con la claqueta) | Llamar a `setLatencySamples()` en `prepareToPlay()`. |
| **Chasquidos en Bypass** | Leve (Molestia auditiva) | Implementar rampa de crossfade suave de 10 ms en lugar de corte seco. |
