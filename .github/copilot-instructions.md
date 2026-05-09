# Proyecto: DistortX - Plugin de Pedal de Distorsión

## Propósito del Proyecto
Desarrollar un plugin de efecto de pedal de distorsión (VST3/AU) en C++ utilizando el framework JUCE, enfocado en emulación de distorsión analógica (Soft y Hard Clipping) y optimizado para Reaper. 

## Stack Tecnológico
- **Lenguaje:** C++17 o superior.
- **Framework:** JUCE.
- **Formatos:** VST3, AU.
- **DAW Principal:** Reaper.
- **Build System:** CMake.

## Requisitos Clave
- **Parámetros DSP:** Drive, Tone (Low-Pass Filter), Level (Volume), Mix (Dry/Wet), Bypass, Tipo de Saturación (Soft/Hard Clipping).
- **Rendimiento:** Baja latencia, optimización de CPU, Zero-latency (o imperceptible) adecuado para tocar en vivo.
- **Fidelidad Audio:** Algoritmos anti-aliasing (Oversampling), parameter smoothing para evitar zipper noise (clicks y pops).
- **Arquitectura:** Separación estricta entre `AudioProcessor` (DSP en hilo de audio sin bloqueos) y `AudioProcessorEditor` (UI en hilo principal), sincronizados mediante `AudioProcessorValueTreeState` (APVTS).

## Reglas de Desarrollo (Para el Agente y el Desarrollador)
- **No locks, no allocations, no blockings:** No permitir bloqueos de memoria (mutexes, reservaciones dinámicas con `new`, lecturas de archivos) dentro del bloque `processBlock()` de DSP.
- **APVTS Mandatorio:** Todos los parámetros automatizables deben estar manejados por `AudioProcessorValueTreeState`.
- **Parameter Smoothing:** Utilizar clases como `juce::SmoothedValue` o `juce::dsp::ProcessorValue` para aplicar suavizado a los parámetros como Ganancia o Frecuencia.
- **Oversampling:** Usar `juce::dsp::Oversampling` en la etapa de modelado no lineal (clipping) para manejar el aliasing inherente a la distorsión.
- **Testing:** Priorizar validaciones teóricas y matemáticas (DSP) antes de escribir código. Probar en Reaper modo UI-off, render offline y procesamiento en tiempo real.

## Fases del Proyecto
1. Configuración y DSP Base (CMake, proceso DSP base, clipping simple).
2. Filtros y Oversampling (Tone control, antialiasing).
3. Estado y APVTS (Sincronización de parámetros y estado con el DAW).
4. Interfaz de Usuario (Perillas, aspecto de pedal, look & feel analógico).
5. Testing, Carga CPU y Debug.
6. Pulido y Despliegue.

*Las sesiones futuras deberán leer estas instrucciones para mantener alineación estricta con la arquitectura de JUCE y los requerimientos del ecosistema de plugins VST3 para la producción de audio profesional.*