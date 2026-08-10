# DistortX - Distortion Pedal Plugin

DistortX es un plugin de audio digital (VST3, AU, Standalone) desarrollado en C++ utilizando el framework JUCE. Su objetivo es emular la saturación armónica y la calidez de un pedal de distorsión analógico clásico, proporcionando algoritmos eficientes para su uso en producción musical, optimizado especialmente para **Reaper**.

## Características Principales

*   **Soft & Hard Clipping:** Algoritmos matemáticos dedicados para lograr desde un "Overdrive" valvular cálido hasta un agresivo "Fuzz/Distortion".
*   **Controles Intuitivos:**
    *   **Gate Threshold / Gate Decay:** Compuerta de ruido al inicio de la cadena DSP para recortar hum y ruido de piso antes de la etapa de ganancia.
    *   **Drive:** Ganancia de entrada hacia la saturación.
    *   **Tone:** Filtro paso-bajo (Low-Pass Filter) para controlar la estridencia y suavizar los agudos.
    *   **Level:** Ajuste de volumen compensatorio en la salida.
    *   **Mix:** Control de mezcla Dry/Wet para el procesamiento paralelo.
*   **Oversampling:** Soporte de anti-aliasing empleando sobremuestreo interno para eliminar los ecos inarmónicos generados por la distorsión no lineal.
*   **Parameter Smoothing:** Transiciones suaves (lock-free) para evitar chasquidos (zipper-noise) al automatizar parámetros.

## Stack Tecnológico

*   **Lenguaje:** C++17
*   **Framework:** JUCE (v7+)
*   **Build System:** CMake
*   **Formatos Compatibles:** VST3, AU, Standalone

## Compilación y Construcción

Para compilar el plugin desde el código fuente, necesitas **CMake** (3.20 o superior) y un compilador de C++ compatible (como MSVC en Windows, o Clang/Apple Clang en macOS).

JUCE se descargará automáticamente como dependencia a través de `FetchContent` de CMake.

```bash
# 1. Clona el repositorio
git clone https://github.com/Jorgez-tech/distortion_pedal.git
cd distortion_pedal

# 2. Configura el proyecto con CMake
cmake -B build

# 3. Compila el plugin en modo Release
cmake --build build --config Release
```

Una vez finalizada la compilación, el archivo `.vst3` se generará en el directorio `build/DistortX_artefacts/Release/VST3/`. Puedes copiar `DistortX.vst3` a tu carpeta de plugins VST3 del sistema para utilizarlo en Reaper o tu DAW preferido.

## Arquitectura
Este proyecto sigue una estricta separación de responsabilidades lock-free para audio en tiempo real:
*   `PluginProcessor`: DSP, algoritmos matemáticos y procesamiento en tiempo real de la señal.
*   `PluginEditor`: Interfaz gráfica de usuario.
*   `AudioProcessorValueTreeState (APVTS)`: Hilo conductor de estado, comunicación y persistencia entre UI, Host (DAW) y el DSP.
*   Cadena DSP actual: `Noise Gate -> Pre-EQ (High-Pass) -> Drive/Waveshaper -> Post-EQ (Tone) -> Level`.

## Licencia
*Proyecto en desarrollo.*
