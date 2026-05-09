# Validación Teórica y de Algoritmos (DSP)

Este documento valida las estrategias algorítmicas y arquitectónicas necesarias para el plugin "DistortX" antes de su implementación en código, garantizando un alto rendimiento y fidelidad de audio en Reaper u otros DAWs.

## 1. Algoritmos de Saturación (Waveshaping)
La distorsión digital se basa en aplicar una Función de Transferencia a la amplitud de las muestras. Define cómo la señal se recorta o comprime al exceder un umbral.

### Soft Clipping (Overdrive)
Recorta los picos suavemente, introduciendo distorsión armónica impar que suena cálida y emula circuitos a válvulas.
- **Algoritmo (Aproximación eficiente hiperbólica):**
  $$ f(x) = \frac{x \cdot \text{drive}}{1 + |x \cdot \text{drive}|} $$
  *También se puede usar `std::tanh` si el rendimiento lo permite, pero la versión fraccional suele ser más económica en CPU.*

### Hard Clipping (Distortion/Fuzz)
Recorte abrupto al alcanzar un límite, generando armónicos agresivos y una onda más cuadrada.
- **Algoritmo:**
  $$
  f(x) =
  \begin{cases}
    1 & \text{si } x \cdot \text{drive} > 1 \\
    -1 & \text{si } x \cdot \text{drive} < -1 \\
    x \cdot \text{drive} & \text{en otro caso}
  \end{cases}
  $$

## 2. Gestión del Anti-Aliasing (Oversampling)
La distorsión introduce nuevas frecuencias altas (armónicos). Si superan la frecuencia de Nyquist, se reflejan hacia el espectro audible creando ruido inarmónico o "aliasing".

- **Validación Algorítmica:** Es imperativo usar **Sobremuestreo (Oversampling)** interno.
- **Implementación propuesta:** Módulo `juce::dsp::Oversampling`. Aplicaremos Upsampling (x2 o x4) antes de las matemáticas de Clipping, procesaremos la saturación y aplicaremos Downsampling con filtrado anti-alias.

## 3. Modulo de Tone (Filtros)
La distorsión incrementa el contenido de altas frecuencias que puede tornar el audio áspero.
- **Algoritmo:** Filtro paso-bajo (Low-Pass Filter) IIR (Infinite Impulse Response) biquadrático.
- **Implementación propuesta:** Uso de `juce::dsp::IIR::Filter`. El control Tone modificará dinámicamente el *cutoff frequency* de este filtro (por ejemplo, barriendo entre 500 Hz y 10 kHz).

## 4. Controladores de Volumen y Suavizado (Parameter Smoothing)
Si el usuario o la automatización en Reaper (envelopes) altera rápidamente la pureza del drive o del volumen de salida, se escucharán chasquidos ("zipper noise").
- **Validación:** No se pueden pasar datos estáticos crudos del slider de la UI directamente al bloque de DSP por cada frame del buffer.
- **Implementación:** Usaremos procesamiento de clase interpoladora, específicamente `juce::SmoothedValue<float>` con una rampa ajustada (~20 ms) en el hilo de DSP para interpolar suavemente los valores objetivo.

## 5. Control Mix (Dry/Wet) y Fase
Mezclar audio sin procesar (Dry) y audio procesado (Wet) para mantener el ataque original y cuerpo de la señal.
- **Algoritmo:** Interpolación Lineal Ponderada.
  $$ \text{Output} = \text{Dry} \cdot (1 - \text{Mix}) + \text{Wet} \cdot \text{Mix} $$
- **Consideraciones arquitectónicas:** Los procesos The IIR filter y el Oversampling inducen un mínimo desfasaje temporal. Aunque en contextos artísticos el "comb-filtering" resultante a veces es deseado o imperceptible, en código riguroso hay que compensar las diferencias de latencia en la ruta Wet frente a la Dry (JUCE lo gestiona o se puede obviar deliberadamente dependiendo del carácter del pedal analógico emulado).
