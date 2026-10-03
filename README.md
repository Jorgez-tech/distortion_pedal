# DistortX — Distortion Pedal Plugin (VST3 / AU / Standalone)

![Build Status](https://github.com/Jorgez-tech/distortion_pedal/actions/workflows/build.yml/badge.svg)
![JUCE Version](https://img.shields.io/badge/JUCE-7.0.9-orange.svg)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Format](https://img.shields.io/badge/Formats-VST3%20%7C%20AU%20%7C%20Standalone-brightgreen.svg)

**DistortX** es un plugin de audio digital desarrollado en **C++17** con el framework **JUCE 7**. Emula la saturación armónica, la dinámica y el grano sonoro de un pedal de distorsión analógico clásico (*stompbox*), optimizado para grabación y directos en **Reaper** con interfaces de baja latencia como la **Behringer UMC22**.

---

## 🎸 Características Principales

* **Interfaz de Pedal Analógico Custom (Fase 4):**
  * Diseño visual stompbox vintage con chasis texturizado en grafito oscuro y tornillos cromados esquineros.
  * Potenciómetros de borde estriado con marcas de graduación perimetrales (*tick marks*) y arco luminoso dinámico.
  * Indicador LED joya luminoso reactivo con efecto de resplandor (*glow*) y bisel metálico.
  * Etiquetas de serigrafía dorada de alta legibilidad para todos los controles.
* **Presets de Fábrica Integrados:** 7 presets calibrados para guitarra y bajo (*Warm Crunch, Tight Modern Lead, Vintage Blues, Heavy Wall of Sound, Clean Warm Boost, Parallel Aggression* y *Default*).
* **Noise Gate Integrado:** Atenuación de siseo y ruido de pastillas (*single-coil hum*) antes de la etapa de ganancia.
* **Saturación Dual (WaveShaping):**
  * **Soft Clipping (Overdrive):** $\tanh(x)$ — armónicos impares suaves y compresión cálida estilo valvular.
  * **Hard Clipping (Distortion/Fuzz):** $\text{clamp}(x, -1, 1)$ — corte abrupto agresivo y rico en armónicos agudos.
* **Pre-HPF (120 Hz):** Filtro pasa-altos para evitar saturación de frecuencias subsónicas y mantener los graves apretados (*tight low-end*).
* **Sobremuestreo Interno x4:** Algoritmo polifásico IIR para eliminar artefactos inarmónicos de *aliasing*.
* **Compensación de Fase y Reporte de Latencia (PDC):** Retardo en señal limpia (`dryBuffer`) para eliminar filtrado en peine (*comb filtering*) y reporte exacto a DAWs como Reaper.
* **Soft-Bypass con Rampa:** Crossfade de 10 ms para activación/desactivación sin chasquidos ni ruidos de conmutación.
* **Control Tone (Post-LPF):** Filtro pasa-bajos biquad (800 Hz – 18 kHz) con cálculo de coeficientes *zero-allocation* en el stack.
* **Procesamiento SIMD:** Mezcla Dry/Wet vectorizada con `FloatVectorOperations` y rampa suave `juce::SmoothedValue`.
* **Lock-free & Real-time Safe:** Diseñado bajo principios de tiempo real estrictos (sin `new`, sin mutexes ni bloqueos en el hilo de audio).

---

## 🎛️ Presets de Fábrica Incluidos

| Preset | Descripción Tonal | Clip Mode | Drive | Tone | Gate | Mix |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **Default** | Ajuste neutro y equilibrado | Soft | 12 dB | 6.5 kHz | -60 dB | 100% |
| **Warm Crunch** | Overdrive valvular con grano cremoso | Soft | 14 dB | 5.5 kHz | -65 dB | 100% |
| **Tight Modern Lead** | Distorsión filosa y definida para solos | Hard | 26 dB | 7.2 kHz | -48 dB | 100% |
| **Vintage Blues** | Overdrive suave y sensible a la dinámica | Soft | 8.5 dB | 4.8 kHz | -75 dB | 100% |
| **Heavy Wall of Sound** | Distorsión masiva de alta ganancia | Hard | 34 dB | 4.2 kHz | -42 dB | 100% |
| **Clean Warm Boost** | Realce limpio con compresión sutil | Soft | 1.5 dB | 12.0 kHz | -80 dB | 100% |
| **Parallel Aggression** | Saturación en paralelo para bajo/baterías | Hard | 28 dB | 8.5 kHz | -60 dB | 45% |

---

## 🎛️ Parámetros y Controles

| Parámetro | ID APVTS | Rango | Unidad | Descripción |
| :--- | :--- | :--- | :--- | :--- |
| **Gate Threshold** | `gateThreshold` | -100.0 a 0.0 | dB | Umbral de activación del noise gate |
| **Gate Decay** | `gateDecay` | 5.0 a 500.0 | ms | Tiempo de liberación de la compuerta |
| **Drive** | `drive` | 0.0 a 36.0 | dB | Ganancia hacia la etapa de distorsión |
| **Tone** | `tone` | 800 a 18000 | Hz | Frecuencia de corte del filtro pasa-bajos |
| **Level** | `level` | -24.0 a +12.0 | dB | Ganancia de salida post-efecto |
| **Mix** | `mix` | 0 a 100 | % | Balance entre señal limpia (Dry) y procesada (Wet) |
| **Saturation Type** | `clipType` | Soft / Hard | Enum | Conmutador entre Overdrive y Hard Distortion |
| **Bypass** | `bypass` | On / Off | Bool | Desactivación del efecto |

---

## 📐 Arquitectura de Procesamiento

```
                 DistortXAudioProcessor (Audio Thread)
                                   │
┌──────────────────────────────────┴──────────────────────────────────┐
│  Audio In ──► [Noise Gate] ──► [Pre-HPF 120Hz] ──► [Drive Gain]     │
│                     │                                     │         │
│               [Dry Buffer]                   [x4 Oversampled Clip]  │
│                     │                                     │         │
│                     │                              [Post-LPF Tone]  │
│                     │                                     │         │
│                     │                             [Level Gain]      │
│                     ▼                                     ▼         │
│              [Delay Match] ────────► [SIMD Dry/Wet Mix]             │
│                                               │                     │
│  Audio Out ◄───────────────────────── [Soft-Bypass]                 │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Compilación e Instalación

### Requisitos
* **CMake 3.20+**
* Compilador **C++17**: Visual Studio 2022 (MSVC) en Windows o Xcode (Apple Clang) en macOS.
* *Nota: JUCE 7.0.9 se descarga y compila automáticamente mediante CMake `FetchContent`.*

### Pasos de Construcción
```bash
# 1. Clonar el repositorio
git clone https://github.com/Jorgez-tech/distortion_pedal.git
cd distortion_pedal

# 2. Configurar el proyecto
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compilar los binarios
cmake --build build --config Release --parallel
```

Los artefactos compilados se ubican en:
* **VST3 (Windows):** `build/DistortX_artefacts/Release/VST3/DistortX.vst3`
* **VST3 (macOS):** `build/DistortX_artefacts/Release/VST3/DistortX.vst3`
* **Standalone:** `build/DistortX_artefacts/Release/Standalone/DistortX.exe`

---

## 🎚️ Configuración Recomendada en Reaper con Behringer UMC22

1. Conectar la guitarra en el **Input 2 (INST 2)** de la Behringer UMC22.
2. En Reaper, crear una pista con entrada **Mono -> Input 2**.
3. Insertar el plugin `DistortX` en la cadena de efectos FX de la pista.
4. Configurar el buffer de audio en Reaper (Preferencias -> Audio -> Device) en **64 o 128 samples** a **44.1 kHz o 48 kHz** con el driver ASIO para latencia imperceptible en tiempo real.

---

## 🖥️ Uso de la Versión Standalone (.exe)

La versión Standalone te permite tocar sin necesidad de abrir Reaper.

**Para configurar tu interfaz de audio (ej. Behringer UMC22):**
1. Abre `DistortX.exe`.
2. En la barra superior de la ventana (donde están los botones de minimizar/cerrar), haz clic en el menú **Options** (Opciones).
3. Selecciona **Audio/MIDI Settings...**.
4. En **Audio Device Type**, selecciona **ASIO** (recomendado para baja latencia).
5. En **Device**, selecciona el driver de tu interfaz (ej. ASIO4ALL o el driver ASIO nativo de Behringer).
6. Activa tu entrada y salida correspondientes, y cierra la ventana de configuración.

---

## 📄 Licencia

Desarrollado por JZ Audio. Licencia MIT / Privada según aplique.
