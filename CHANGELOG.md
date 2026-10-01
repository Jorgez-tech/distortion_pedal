# Changelog

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y este proyecto se adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-10-01

### Added
- **Banco de Presets de Fábrica (Fase 6):**
  - Integración nativa de gestión de programas en `DistortXAudioProcessor` con 7 presets calibrados:
    1. *Default* (Configuración balanceada estándar)
    2. *Warm Crunch* (Overdrive valvular cálido con saturación suave y presencia)
    3. *Tight Modern Lead* (Distorsión cortante para solos de guitarra con puerta de ruido rápida)
    4. *Vintage Blues Overdrive* (Saturación ligera y dinámica de toque estilo Texas Blues)
    5. *Heavy Wall of Sound* (Distorsión extrema de alta ganancia para riffs pesados)
    6. *Clean Warm Boost* (Realce limpio con calidez armónica y recorte de agudos ásperos)
    7. *Parallel Aggression* (Saturación paralela al 45% Mix optimizada para bajo y percusión)
  - **Selector Visual de Presets en la GUI:** ComboBox estilizado en la placa de marca dorada superior del editor con carga instantánea y persistencia de sesión XML (`currentProgram`).
  - **Suite de Pruebas Unitarias Ampliada:** Verificación automatizada de carga de banco y consistencia de rangos en Catch2 (13 casos de prueba y 21.917 aserciones pasando al 100%).
- **Lanzamiento Oficial v1.0.0:** Generación de binarios Release optimizados con LTO para VST3 y Standalone.

## [0.4.0] - 2026-10-01

### Added
- **Suite Completa de Pruebas Automatizadas (Fase 5 - Catch2 v3):**
  - Target ejecutable `DistortX_Tests` integrado en CMake mediante `FetchContent` con Catch2 v3.5.2.
  - **`DSPCurvesTests`:** Verificación rigurosa de simetría impar, monotonía, límites asintóticos en $[-1.0, 1.0]$ para Soft Clipping ($\tanh$) y corte lineal/saturado para Hard Clipping ($\text{clamp}$).
  - **`FilterStabilityTests`:** Comprobación de atenuación DC en HPF 120 Hz, barrido continuo del LPF Tone (800 Hz a 18 kHz) sin generar `NaN` ni `Inf`, y validación en 5 frecuencias de muestreo (44.1, 48, 88.2, 96 y 192 kHz).
  - **`LatencyPDCTests`:** Comprobación del reporte de latencia PDC al host y suma coherente en fase sin cancelación (*comb filtering*) en posición intermedia de `Mix`.
  - **`RealtimeStressTests`:** Pruebas de estabilidad con tamaños de buffer desde 32 hasta 2048 muestras, verificación de crossfade suave en Soft-Bypass y atenuación de ruido por debajo del umbral en el Noise Gate.
  - **`APVTSStateTests`:** Verificación de existencia de los 8 parámetros y serialización/deserialización XML completa sin pérdida de estado.
  - **Filtrado Multicanal Estéreo Mejorado:** Migración de los filtros IIR a `juce::dsp::ProcessorDuplicator` con estado independiente por canal en `DistortionChain`.
- **Resultados de Validación:** 100% de tests superados (21.818 aserciones en 12 casos de prueba).

## [0.3.1] - 2026-10-01

### Fixed
- **DSP-02 — Zero-Allocation garantizado en WaveShaper:** Las lambdas con captura en `setClipMode()` han sido sustituidas por punteros a funciones libres estáticas `softClip` / `hardClip` (`noexcept`). Esto elimina cualquier riesgo de asignación de heap al cambiar el modo de saturación, independientemente del compilador o tamaño del SBO de `std::function`.
- **DSP-03 — Doble inicialización de filtros corregida:** Los comentarios y el flujo de `prepareToPlay()` se clarificaron para documentar que la pre-asignación de coeficientes en el message thread solo sirve para inicializar el puntero (evitar null-ptr), mientras que los valores reales son provistos por `filtersNeedRefresh=true` en el primer `processBlock`. Se elimina ambigüedad de propósito.
- **DSP-01 — `dryDelayLine` documentado como invariante:** El tamaño de 1024 muestras queda comentado con justificación técnica (margen ×40 sobre la latencia máxima del IIR poliphase x4 a 192 kHz).
- **UI-01 — Inconsistencia de altura de cabecera resuelta:** Se introduce `constexpr kHeaderHeight = 52` compartido entre `paint()` y `resized()`. Antes `paint()` usaba 52 px y `resized()` 46 px, lo que podía desplazar las etiquetas de sección por encima de los controles.
- **CMK-01 — Formato AU condicional en CMakeLists:** El formato `AU` (Audio Unit) se activa únicamente en macOS mediante un generador expression `$<$<PLATFORM_ID:Darwin>:AU>`. En builds de Windows queda `VST3 Standalone` sin formato huérfano.

## [0.3.0] - 2026-09-19

### Added
- **UI LookAndFeel Analógico (Fase 4):**
  - Clase `DistortXLookAndFeel` con potenciómetros rotatorios estilo stompbox analógico.
  - Graduaciones perimetrales (*tick marks*), punteros luminosos de alto contraste y borde estriado.
  - Chasis metálico oscuro con serigrafía dorada, tornillos esquineros y separadores de sección.
  - Indicador LED joya luminoso reactivo al estado de Bypass/On.
  - Etiquetas descriptivas para todos los potenciómetros (`THRESHOLD`, `GATE DECAY`, `DRIVE`, `TONE`, `LEVEL`, `MIX`, `CLIP MODE`).
- **Saneamiento DSP & Zero-Allocation:**
  - Sustitución de `makeLowPass` dinámico por cálculo de coeficientes de filtro biquad en stack sin reservas de memoria en el hilo de audio.
  - **PDC Latency Reporting:** Reporte de latencia de oversampling al DAW (Reaper) con `setLatencySamples()`.
  - **Alineación de Fase en Dry/Wet:** Retardo compensatorio en `dryBuffer` para evitar filtrado en peine (*comb filtering*).
  - **Soft-Bypass con Rampa:** Crossfade de 10 ms para activación/desactivación sin chasquidos ni ruidos parásitos.

### Planned / Roadmap
- **Gestor de Presets de Fábrica:** Presets preconfigurados (*Clean Boost, Warm Crunch, Heavy Metal, Lead Scream, Fuzz Mania*).
- **Suite de Pruebas Automatizadas (Catch2):** Tests unitarios de validación matemática y respuesta en frecuencia.

## [0.2.0] - 2026-09-19

### Added
- **Noise Gate Integrado:** Atenuador de ruido de entrada con parámetros `gateThreshold` (-100 a 0 dB) y `gateDecay` (5 a 500 ms).
- **Pre-Filtro Pasa-Altos (HPF):** Corte fijo a 120 Hz previo a la ganancia para limpiar graves.
- **OversampledClipper (x4):** Módulo de sobremuestreo x4 polifásico IIR con algoritmos Soft Clipping (`tanh`) y Hard Clipping (`clamp`).
- **Post-Filtro Tone (LPF):** Filtro pasa-bajos biquad regulable (800 Hz a 18 kHz).
- **Control Level & Mix:** Volumen de salida y mezcla Dry/Wet acelerada por SIMD (`FloatVectorOperations`) con rampa anti-zipper `juce::SmoothedValue`.
- **APVTS Completo:** 8 parámetros automatizables expuestos al host con persistencia de sesión XML (`getStateInformation` / `setStateInformation`).
- **CI/CD con GitHub Actions:** Flujo de trabajo automatizado para compilación cruzada en Windows (MSVC) y macOS (Apple Clang) con publicación de artefactos VST3.

## [0.1.0] - 2026-09-19

### Added
- Estructura inicial del proyecto con CMake 3.20+ y JUCE 7.0.9 mediante `FetchContent`.
- Configuración para generación de targets VST3, AU y Standalone.
- Clases base `DistortXAudioProcessor` y `DistortXAudioProcessorEditor`.
- Documentación inicial de especificación y arquitectura en `docs/project_plan.md` y `docs/validation.md`.
