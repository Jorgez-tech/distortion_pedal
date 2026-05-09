# Plan de Proyecto: Plugin de Pedal de Distorsión "DistortX"

## 1. Resumen Ejecutivo
El objetivo de este proyecto es diseñar, desarrollar y lanzar un plugin de audio digital (formato VST3 y AU) que emule un pedal de distorsión analógico. El plugin estará optimizado para usarse en Reaper y proporcionará a los músicos y productores un efecto de saturación cálida a distorsión agresiva con una interfaz fácil de usar. Se construirá utilizando el framework JUCE para garantizar soporte multiplataforma.

## 2. Tecnologías y Herramientas (Stack Tecnológico)
- **Lenguaje de Programación:** C++ (Estándar C++17 o superior).
- **Framework DSP y GUI:** JUCE Framework.
- **Entornos de Desarrollo (IDE):** Visual Studio (Windows) / Xcode (macOS) / VS Code.
- **Gestor de Construcción:** CMake y/o Projucer.
- **DAW de Pruebas Principal:** Reaper.
- **Control de Versiones:** Git y GitHub.

## 3. Especificación de Requisitos

### 3.1 Requisitos Funcionales
- **Algoritmo de Distorsión:** Debe implementar al menos dos tipos de recortes (clipping): "Soft Clipping" (estilo Overdrive) y "Hard Clipping" (estilo Distortion).
- **Controles de Interfaz:**
  - **Drive/Gain:** Controla la cantidad de señal de entrada que se satura.
  - **Tone/EQ:** Filtro paso bajo (Low-Pass Filter) para atenuar frecuencias altas y ajustar el color.
  - **Level/Volume:** Ajuste de ganancia para compensar el aumento de volumen.
  - **Mix (Dry/Wet):** Permite mezclar la señal limpia original con la distorsionada.
  - **Tipos de Saturación:** Interruptor para alternar entre "Soft Clipping" y "Hard Clipping".
  - **Bypass:** Botón para encender/apagar el efecto sin producir chasquidos.

### 3.2 Requisitos No Funcionales
- **Rendimiento / CPU:** El DSP debe ser extremadamente eficiente.
- **Baja Latencia:** Latenza cero o imperceptible para poder tocar en vivo.
- **Compatibilidad de Formatos:** Compilable como VST3 (Windows/Mac) y AU (Mac).
- **Automatización:** Todos los parámetros deben ser expuestos al host (Reaper) para automatización.
- **Persistencia (State Management):** El plugin debe guardar los parámetros en el proyecto del DAW.

## 4. Arquitectura del Software (Modelo JUCE)
El proyecto se dividirá en dos clases principales:
- **AudioProcessor (Backend DSP):** Maneja la lógica matemática del audio y procesa la señal en tiempo real sin bloqueos. Gestiona el APVTS.
- **AudioProcessorEditor (Frontend UI):** Maneja los gráficos, eventos de interfaz y la comunicación fluida con el `AudioProcessor`.
- **AudioProcessorValueTreeState (APVTS):** Enlaza los parámetros de la UI con las variables del DSP.

## 5. Cronograma e Hitos del Proyecto
- **Fase 1: Configuración e Infraestructura** (Semanas 1-2): Setup del repositorio, CMake, JUCE y procesos base.
- **Fase 2: DSP - Desarrollo del Algoritmo Analítico** (Semanas 3-4): Rutina de audio, algoritmos de saturación, Tone y Oversampling.
- **Fase 3: Gestión de Parámetros** (Semana 5): Creación del APVTS y sincronización de variables con el VST3.
- **Fase 4: Desarrollo de Interfaz de Usuario UI** (Semanas 6-7): Diseño personalizado, sliders y gráficos en JUCE.
- **Fase 5: Testing, Mejoras y Optimización** (Semana 8): Pruebas en Reaper con distintos buffers y uso de Oversampling interno.
- **Fase 6: Release** (Semana 9): Refactorización, builds de producción y creación de instaladores.

## 6. Riesgos Potenciales y Mitigación
- **Chasquidos de Audio ("Zippering"):** Usar interpolación lineal o envolventes (`SmoothedValue` en JUCE).
- **Aliasing:** Usar sobre-muestreo (`dsp::Oversampling`) en las etapas de ganancia no-lineal.
- **Acceso concurrente GUI vs DSP:** Estricta adherencia a "No locks, no allocations, no blockings" en el thread principal de audio. Uso de APVTS para transmisión de estado.
