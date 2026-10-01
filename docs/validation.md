# Validación Teórica y de Algoritmos (DSP) — DistortX

Este documento valida las estrategias matemáticas, algorítmicas y de arquitectura en tiempo real para el plugin **DistortX**, garantizando máxima fidelidad de audio y cero bloqueos (*zero-allocation / zero-lock*) en DAWs de baja latencia como **Cockos Reaper** y hardware como la interfaz **Behringer UMC22**.

---

## 1. Algoritmos de Saturación (Waveshaping)

La distorsión analógica en pedales tipo stompbox se modela aplicando funciones de transferencia no lineales sobre la amplitud de la señal.

### 1.1 Soft Clipping (Emulación Valvular / Overdrive)
Modela la compresión gradual de válvulas o diodos de silicio en bucle de realimentación (estilo Tube Screamer).
* **Función de Transferencia:**
  $$ f(x) = \tanh(x) $$
* **Propiedades Armónicas:** Genera predominantemente armónicos impares (3er, 5to orden) con atenuación armónica suave y compresión natural en picos elevados.
* **Comportamiento Asintótico:** Para valores grandes de entrada, $f(x) \to \pm 1$, garantizando que nunca se exceda la escala completa digital (*0 dBFS*).

### 1.2 Hard Clipping (Emulación Diodos a Masa / Distortion / Fuzz)
Modela el recorte abrupto producido por diodos conectados a masa (estilo DS-1 o RAT).
* **Función de Transferencia:**
  $$
  f(x) = \text{clamp}(x, -1.0, 1.0) =
  \begin{cases}
    1.0 & \text{si } x > 1.0 \\
    -1.0 & \text{si } x < -1.0 \\
    x & \text{en otro caso}
  \end{cases}
  $$
* **Propiedades Armónicas:** Produce transiciones angulares abruptas que inyectan armónicos de alta frecuencia, generando un timbre agresivo, filoso y definido.

---

## 2. Gestión de Anti-Aliasing (Oversampling x4)

### 2.1 El Fenómeno del Aliasing
La no linealidad de la saturación genera armónicos en múltiplos enteros de la frecuencia fundamental ($f_k = k \cdot f_0$). Cuando estos armónicos superan la frecuencia de Nyquist ($f_N = f_s / 2$), se reflejan hacia el espectro audible como frecuencias inarmónicas no musicales (*aliasing*).

### 2.2 Estrategia de Sobremuestreo
* **Módulo:** `juce::dsp::Oversampling<float>` con factor de 2 etapas ($2^2 = 4\times$).
* **Filtro:** `filterHalfBandPolyphaseIIR` (Filtro Polifásico Half-Band IIR).
* **Ventajas:**
  1. Excelente atenuación en banda de rechazo (> 90 dB).
  2. Carga de CPU significativamente menor comparada con filtros FIR simétricos.
  3. Desplaza la frecuencia de Nyquist efectiva de $22.05\text{ kHz}$ a $88.2\text{ kHz}$ (a $44.1\text{ kHz}$ de entrada), asegurando que los primeros 7 armónicos de notas agudas de guitarra no se reflejen en la banda audible.

---

## 3. Diseño de Filtros y Garantía Zero-Allocation en Tiempo Real

### 3.1 El Problema de `IIR::Coefficients::makeLowPass`
En implementaciones ingenuas de JUCE, el método:
```cpp
*filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cutoffHz, q);
```
crea internamente un `new juce::dsp::IIR::Coefficients` en el montículo (Heap). Cuando el usuario modula el control `Tone` o el DAW ejecuta una automatización, este `new` se ejecuta en el **Audio Thread**, violando los principios de tiempo real y provocando potenciales *xruns*.

### 3.2 Solución Matemática Zero-Allocation
Para calcular los coeficientes de un filtro biquad analógico bilineal (Robert Bristow-Johnson Audio EQ Cookbook) sin ninguna asignación dinámica:

$$ \omega_0 = 2\pi \frac{f_c}{f_s}, \quad \alpha = \frac{\sin(\omega_0)}{2Q} $$

Para un **Low-Pass Filter (LPF) de 2do Orden**:
$$ b_0 = \frac{1 - \cos(\omega_0)}{2}, \quad b_1 = 1 - \cos(\omega_0), \quad b_2 = \frac{1 - \cos(\omega_0)}{2} $$
$$ a_0 = 1 + \alpha, \quad a_1 = -2\cos(\omega_0), \quad a_2 = 1 - \alpha $$

Normalizando por $a_0$:
$$ c_0 = b_0 / a_0, \quad c_1 = b_1 / a_0, \quad c_2 = b_2 / a_0, \quad c_3 = a_1 / a_0, \quad c_4 = a_2 / a_0 $$

Los coeficientes se copian directamente en la estructura de memoria existente del filtro sin invocar `new` ni `delete`.

Alternativamente, el uso de `juce::dsp::StateVariableTPTFilter` (Topology-Preserving Transform) permite modular `setCutoffFrequency()` en tiempo real de forma continua con cero reservas y estabilidad numérica incondicional.

---

## 4. Compensación de Retardo y Alineación de Fase en Mix Dry/Wet

### 4.1 Retardo del Sobremuestreo y Filtrado
El proceso de sobremuestreo e interpolación introduce una latencia intrínseca $D_{os} = \tau_{filter}$ muestras.

### 4.2 Efecto de Filtrado en Peine (Comb Filtering)
Si la señal limpia $x[n]$ y la señal procesada $y[n - D_{os}]$ se suman directamente en el control Mix:
$$ s[n] = (1 - \text{Mix}) \cdot x[n] + \text{Mix} \cdot y[n - D_{os}] $$
La diferencia temporal $D_{os}$ genera muescas destructivas en la respuesta en frecuencia:
$$ f_{\text{notch}} = \frac{(2k + 1)}{2 D_{os}} \cdot f_s $$

### 4.3 Mitigación Técnica
1. **Línea de Retardo en Rama Limpia:** Se retarda la señal `dryBuffer` exactamente $D_{os}$ muestras antes de la suma vectorial SIMD.
2. **Reporte PDC a Reaper:** Se invoca `setLatencySamples(roundToInt(oversamplingLatency))` en `prepareToPlay()` para que el motor de Reaper alinee automáticamente todas las pistas en la sesión.

---

## 5. Soft-Bypass y Prevención de Transitorios (De-Clicking)

Para evitar chasquidos (*pops / clicks*) al conmutar el estado de Bypass:
* No se realiza un corte abrupto de procesamiento (`return;`).
* Se utiliza una rampa de fundido cruzado (*crossfade*) de $10\text{ ms}$ gestionada por `juce::SmoothedValue<float>`.
* La cadena DSP permanece activa en estado durmiente mientras el bypass está en 100%, permitiendo reingresar sin saltos de fase.

---

## 6. Metodología de Pruebas y Validación Automatizada (Fase 5 - Catch2)

Para garantizar la robustez industrial del plugin en entornos de grabación en directo con **Cockos Reaper** y la interfaz **Behringer UMC22**, la suite de pruebas unitarias aborda 5 vectores críticos:

1. **Curvas Matemáticas de Saturación:**
   * Verificación de simetría impar en Soft Clipping: $f(-x) = -f(x)$ con $f(x) = \tanh(x)$.
   * Comprobación de límites estrictos en Hard Clipping: $\forall x \in \mathbb{R}, |f(x)| \le 1.0$.
2. **Respuesta en Frecuencia y Filtros:**
   * Atenuación a -3 dB en corte de HPF 120 Hz.
   * Modulación continua de LPF Tone (800 Hz – 18 kHz) sin NaN, inf ni discontinuidades.
3. **Alineación de Fase y Latencia (PDC):**
   * Verificación de retardo de `dryDelayLine` coincidente exactamente con `oversampling->getLatencyInSamples()`.
   * Ausencia de comb filtering a $50\%$ Mix con señal sinusoidal en fase.
4. **Pruebas de Estrés de Buffer en Tiempo Real:**
   * Estabilidad con tamaños de bloque extremos: $32, 64, 128, 256, 512, 1024, 2048$ muestras.
   * Manejo seguro de sample rates estándar: $44.1, 48, 88.2, 96, 192\text{ kHz}$.
5. **Persistencia de Parámetros (APVTS):**
   * Serialización y deserialización idéntica mediante XML sin pérdida de precisión.

