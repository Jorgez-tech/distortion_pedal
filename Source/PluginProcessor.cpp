#include "PluginProcessor.h"
#include "PluginEditor.h"

void DistortXAudioProcessor::OversampledClipper::prepare (const juce::dsp::ProcessSpec& spec)
{
    // FIREquiripple en lugar de PolyphaseIIR: los filtros IIR tienen polos de alta Q
    // cerca de los ~11kHz que resuenan con el ataque de la púa (energía broadband),
    // produciendo el "chicharreo" audible. Los filtros FIR no tienen polos y no pueden
    // resonar — solución estándar en plugins de guitarra profesionales.
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
    oversampling->reset();
    oversampling->initProcessing (spec.maximumBlockSize);

    juce::dsp::ProcessSpec upsampledSpec { spec.sampleRate * oversampling->getOversamplingFactor(),
                                           static_cast<juce::uint32> (spec.maximumBlockSize * oversampling->getOversamplingFactor()),
                                           spec.numChannels };
    setClipMode (currentMode >= 0 ? currentMode : 0);
    waveshaper.prepare (upsampledSpec);
}

void DistortXAudioProcessor::OversampledClipper::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();
    waveshaper.reset();
}

void DistortXAudioProcessor::OversampledClipper::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    if (oversampling == nullptr)
        return;

    auto oversampledBlock = oversampling->processSamplesUp (context.getOutputBlock());
    juce::dsp::ProcessContextReplacing<float> oversampledContext (oversampledBlock);
    waveshaper.process (oversampledContext);
    oversampling->processSamplesDown (context.getOutputBlock());
}

// Funciones libres estáticas para el WaveShaper — garantizan zero-allocation
// en el audio thread sin depender del SBO de std::function (DSP-02).
static float softClip (float x) noexcept { return std::tanh (x); }
static float hardClip (float x) noexcept { return juce::jlimit (-1.0f, 1.0f, x); }

void DistortXAudioProcessor::OversampledClipper::setClipMode (int newMode) noexcept
{
    // Llamado exclusivamente desde processBlock() en el audio thread.
    // No se necesita atomic — currentMode es privado al audio thread.
    currentMode = newMode;

    if (newMode == 0)
    {
        // Soft Clipping: tanh — armónicos impares, calidez analógica.
        // Puntero a función libre: zero-allocation garantizado.
        waveshaper.functionToUse = softClip;
        return;
    }

    // Hard Clipping: clamp — agresivo, armónicos pares + impares.
    waveshaper.functionToUse = hardClip;
}

DistortXAudioProcessor::DistortXAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    gateThresholdParam = apvts.getRawParameterValue ("gateThreshold");
    gateDecayParam = apvts.getRawParameterValue ("gateDecay");
    driveParam = apvts.getRawParameterValue ("drive");
    toneParam = apvts.getRawParameterValue ("tone");
    levelParam = apvts.getRawParameterValue ("level");
    mixParam = apvts.getRawParameterValue ("mix");
    bypassParam = apvts.getRawParameterValue ("bypass");
    clipTypeParam = apvts.getRawParameterValue ("clipType");
}

DistortXAudioProcessor::~DistortXAudioProcessor()
{
}

const juce::String DistortXAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

juce::AudioProcessorValueTreeState::ParameterLayout DistortXAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gateThreshold", 1 }, "Gate Threshold",
                                                                    juce::NormalisableRange<float> (-100.0f, 0.0f, 0.1f), -60.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gateDecay", 1 }, "Gate Decay",
                                                                    juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.5f), 120.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "drive", 1 }, "Drive",
                                                                    juce::NormalisableRange<float> (0.0f, 36.0f, 0.01f), 12.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tone", 1 }, "Tone",
                                                                    juce::NormalisableRange<float> (800.0f, 18000.0f, 1.0f, 0.35f), 6500.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "level", 1 }, "Level",
                                                                    juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                                                                    juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "clipType", 1 }, "Saturation Type",
                                                                     juce::StringArray { "Soft", "Hard" }, 0));

    return { params.begin(), params.end() };
}

void DistortXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate, std::memory_order_release);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (juce::jmax (1, getTotalNumInputChannels()));

    // Pre-asignar los objetos Coefficients en el message thread para que el puntero
    // esté inicializado con orden 2 antes de prepare() y reset().
    dspChain.get<preEQIndex>().state  = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 120.0f, 0.70710678f);
    dspChain.get<postEQIndex>().state = juce::dsp::IIR::Coefficients<float>::makeLowPass  (sampleRate, 6500.0f, 0.70710678f);

    dspChain.prepare (spec);
    dspChain.reset();
    dspChain.get<gateIndex>().setRatio (100.0f);
    // Attack de 25ms: evita el gate chattering (traqueteo) en cuerdas al aire.
    // Con 2ms el gate podía responder a frecuencias de hasta ~500Hz, abriendo y
    // cerrando al ritmo de la frecuencia fundamental de la cuerda (ej: 110Hz en
    // cuerda A), lo que causaba el "chicharreo" audible al tocar.
    // Con 25ms el gate no puede responder más rápido que ~40Hz (inaudible como
    // modulación), pero sigue siendo lo suficientemente rápido para el ataque
    // natural de una guitarra (imperceptible para el oído).
    dspChain.get<gateIndex>().setAttack (25.0f);
    dspChain.get<gateIndex>().setThreshold (juce::jlimit (-100.0f, 0.0f, gateThresholdParam->load (std::memory_order_relaxed)));
    dspChain.get<gateIndex>().setRelease (juce::jlimit (5.0f, 500.0f, gateDecayParam->load (std::memory_order_relaxed)));
    dspChain.get<preGainIndex>().setRampDurationSeconds (0.02);
    dspChain.get<outputGainIndex>().setRampDurationSeconds (0.02);

    // Configurar reporte de latencia a Reaper (PDC) y compensación de fase en dryBuffer
    const auto clipperLatency = dspChain.get<clipperIndex>().getLatencyInSamples();
    setLatencySamples (juce::roundToInt (clipperLatency));

    dryDelayLine.prepare (spec);
    dryDelayLine.reset();
    dryDelayLine.setDelay (clipperLatency);

    mixSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, mixParam->load (std::memory_order_relaxed)));

    bypassSmoothed.reset (sampleRate, 0.01); // 10ms rampa de soft-bypass
    bypassSmoothed.setCurrentAndTargetValue (bypassParam->load (std::memory_order_relaxed) > 0.5f ? 1.0f : 0.0f);

    dryBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);
    rawInputBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);

    lastToneCutoffHz = -1.0f;
    lastDriveDb = -1000.0f;
    lastLevelDb = -1000.0f;
    lastGateThresholdDb = -1000.0f;
    lastGateDecayMs = -1.0f;
    lastClipMode = -1;
    filtersNeedRefresh.store (true, std::memory_order_release);
}

void DistortXAudioProcessor::releaseResources()
{
    dspChain.reset();
    dryDelayLine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DistortXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void DistortXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    const int numActiveChannels = juce::jmax (1, totalNumInputChannels);

    for (auto i = numActiveChannels; i < buffer.getNumChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numActiveChannels > dryBuffer.getNumChannels() || numSamples > dryBuffer.getNumSamples())
    {
        jassertfalse;
        return;
    }

    const bool isBypassed = bypassParam->load (std::memory_order_relaxed) > 0.5f;
    bypassSmoothed.setTargetValue (isBypassed ? 1.0f : 0.0f);

    // Si el bypass está en 100% y no está en transición, salida directa sin costo de CPU
    if (bypassSmoothed.getCurrentValue() >= 1.0f && !bypassSmoothed.isSmoothing())
        return;

    // Guardar una copia cruda de la entrada para el crossfade del soft-bypass
    for (int channel = 0; channel < numActiveChannels; ++channel)
        juce::FloatVectorOperations::copy (rawInputBuffer.getWritePointer (channel), buffer.getReadPointer (channel), numSamples);

    const auto driveDb = driveParam->load (std::memory_order_relaxed);
    const auto levelDb = levelParam->load (std::memory_order_relaxed);
    const auto mixValue = juce::jlimit (0.0f, 1.0f, mixParam->load (std::memory_order_relaxed));
    const auto toneCutoff = juce::jlimit (800.0f, 18000.0f, toneParam->load (std::memory_order_relaxed));
    const auto gateThresholdDb = juce::jlimit (-100.0f, 0.0f, gateThresholdParam->load (std::memory_order_relaxed));
    const auto gateDecayMs = juce::jlimit (5.0f, 500.0f, gateDecayParam->load (std::memory_order_relaxed));

    if (std::abs (gateThresholdDb - lastGateThresholdDb) > 0.05f)
    {
        dspChain.get<gateIndex>().setThreshold (gateThresholdDb);
        lastGateThresholdDb = gateThresholdDb;
    }

    if (std::abs (gateDecayMs - lastGateDecayMs) > 0.5f)
    {
        dspChain.get<gateIndex>().setRelease (gateDecayMs);
        lastGateDecayMs = gateDecayMs;
    }

    if (std::abs (driveDb - lastDriveDb) > 0.001f)
    {
        dspChain.get<preGainIndex>().setGainDecibels (driveDb);
        lastDriveDb = driveDb;
    }

    if (std::abs (levelDb - lastLevelDb) > 0.001f)
    {
        dspChain.get<outputGainIndex>().setGainDecibels (levelDb);
        lastLevelDb = levelDb;
    }
    const auto clipMode = static_cast<int> (clipTypeParam->load (std::memory_order_relaxed));
    if (clipMode != lastClipMode)
    {
        dspChain.get<clipperIndex>().setClipMode (clipMode);
        lastClipMode = clipMode;
    }
    mixSmoothed.setTargetValue (mixValue);

    if (filtersNeedRefresh.exchange (false, std::memory_order_acq_rel))
    {
        updateHighPassCoefficients();
        lastToneCutoffHz = -1.0f;
    }

    if (std::abs (toneCutoff - lastToneCutoffHz) > 0.5f)
    {
        updateLowPassCoefficients (toneCutoff);
        lastToneCutoffHz = toneCutoff;
    }

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), 
                                        static_cast<size_t> (numActiveChannels), 
                                        static_cast<size_t> (numSamples));
    juce::dsp::ProcessContextReplacing<float> context (block);

    // 1. Noise Gate
    dspChain.get<gateIndex>().process (context);

    // 2. Copiar señal limpia y retrasarla para alinear fase con el Oversampling
    for (int channel = 0; channel < numActiveChannels; ++channel)
        juce::FloatVectorOperations::copy (dryBuffer.getWritePointer (channel), buffer.getReadPointer (channel), numSamples);

    juce::dsp::AudioBlock<float> dryBlock (dryBuffer.getArrayOfWritePointers(),
                                           static_cast<size_t> (numActiveChannels),
                                           static_cast<size_t> (numSamples));
    juce::dsp::ProcessContextReplacing<float> dryContext (dryBlock);
    dryDelayLine.process (dryContext);

    // 3. Cadena de Distorsión
    dspChain.get<preEQIndex>().process (context);
    dspChain.get<preGainIndex>().process (context);
    dspChain.get<clipperIndex>().process (context);
    dspChain.get<postEQIndex>().process (context);
    dspChain.get<outputGainIndex>().process (context);

    float* wetPointers[2] {};
    const float* dryPointers[2] {};
    for (int channel = 0; channel < numActiveChannels; ++channel)
    {
        wetPointers[channel] = buffer.getWritePointer (channel);
        dryPointers[channel] = dryBuffer.getReadPointer (channel);
    }

    // 4. Dry/Wet SIMD Mix con alineación de fase
    if (mixSmoothed.isSmoothing())
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float mix = mixSmoothed.getNextValue();
            const float dryGain = 1.0f - mix;
            for (int ch = 0; ch < numActiveChannels; ++ch)
                wetPointers[ch][sample] = dryGain * dryPointers[ch][sample] + mix * wetPointers[ch][sample];
        }
    }
    else
    {
        const float mix = mixSmoothed.getNextValue();
        const float dryGain = 1.0f - mix;
        for (int ch = 0; ch < numActiveChannels; ++ch)
        {
            juce::FloatVectorOperations::multiply (wetPointers[ch], mix, numSamples);
            juce::FloatVectorOperations::addWithMultiply (wetPointers[ch], dryPointers[ch], dryGain, numSamples);
        }
    }

    // 5. Soft-Bypass Crossfade (De-Clicking)
    if (bypassSmoothed.isSmoothing() || bypassSmoothed.getCurrentValue() > 0.0f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float bypassAmount = bypassSmoothed.getNextValue();
            const float activeAmount = 1.0f - bypassAmount;
            for (int ch = 0; ch < numActiveChannels; ++ch)
            {
                const float raw = rawInputBuffer.getReadPointer (ch)[sample];
                const float proc = wetPointers[ch][sample];
                wetPointers[ch][sample] = bypassAmount * raw + activeAmount * proc;
            }
        }
    }

    // Mono-to-Stereo routing: si procesamos 1 canal pero la salida espera 2,
    // copiamos la señal procesada del canal izquierdo al derecho.
    if (numActiveChannels == 1 && totalNumOutputChannels == 2)
    {
        buffer.copyFrom (1, 0, buffer.getReadPointer (0), numSamples);
    }
}

void DistortXAudioProcessor::updateHighPassCoefficients() noexcept
{
    setBiquadCoefficients (dspChain.get<preEQIndex>(), currentSampleRate.load (std::memory_order_acquire), 120.0f, 0.70710678f, true);
}

void DistortXAudioProcessor::updateLowPassCoefficients (float cutoffHz) noexcept
{
    setBiquadCoefficients (dspChain.get<postEQIndex>(), currentSampleRate.load (std::memory_order_acquire), cutoffHz, 0.70710678f, false);
}

void DistortXAudioProcessor::setBiquadCoefficients (Filter& filter, double sampleRate, float cutoffHz, float q, bool highPass) noexcept
{
    const auto nyquist = static_cast<float> (sampleRate * 0.5);
    const auto clampedCutoff = juce::jlimit (20.0f, nyquist - 1.0f, cutoffHz);

    if (filter.state == nullptr)
    {
        filter.state = highPass
            ? juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, clampedCutoff, q)
            : juce::dsp::IIR::Coefficients<float>::makeLowPass  (sampleRate, clampedCutoff, q);
        return;
    }

    // operator= con ArrayCoefficients utiliza clearQuick() sobre la memoria pre-asignada
    // normalizando automáticamente por a0 con cero reservas dinámicas en el audio thread.
    if (highPass)
        *filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, clampedCutoff, q);
    else
        *filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass  (sampleRate, clampedCutoff, q);
}

bool DistortXAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DistortXAudioProcessor::createEditor()
{
    return new DistortXAudioProcessorEditor (*this);
}

const std::vector<DistortXAudioProcessor::Preset>& DistortXAudioProcessor::getFactoryPresets()
{
    static const std::vector<Preset> presets = {
        { "Default",                -60.0f, 120.0f, 12.0f, 6500.0f,  0.0f, 1.0f,  false, 0 },
        { "Warm Crunch",            -65.0f, 100.0f, 14.0f, 5500.0f,  1.0f, 1.0f,  false, 0 },
        { "Tight Modern Lead",      -48.0f,  60.0f, 26.0f, 7200.0f,  0.0f, 1.0f,  false, 1 },
        { "Vintage Blues Overdrive", -75.0f, 150.0f,  8.5f, 4800.0f,  3.0f, 1.0f,  false, 0 },
        { "Heavy Wall of Sound",    -42.0f,  50.0f, 34.0f, 4200.0f, -2.0f, 1.0f,  false, 1 },
        { "Clean Warm Boost",       -80.0f, 120.0f,  1.5f, 12000.0f, 6.0f, 1.0f,  false, 0 },
        { "Parallel Aggression",    -60.0f, 100.0f, 28.0f, 8500.0f, -3.0f, 0.45f, false, 1 }
    };
    return presets;
}

int DistortXAudioProcessor::getNumPrograms()
{
    return static_cast<int> (getFactoryPresets().size());
}

int DistortXAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

const juce::String DistortXAudioProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    if (juce::isPositiveAndBelow (index, static_cast<int> (presets.size())))
        return presets[static_cast<size_t> (index)].name;
    return {};
}

void DistortXAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void DistortXAudioProcessor::loadPreset (int presetIndex)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (presetIndex, static_cast<int> (presets.size())))
        return;

    currentProgram = presetIndex;
    const auto& p = presets[static_cast<size_t> (presetIndex)];

    auto setParam = [this] (const juce::String& paramID, float val)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (paramID)))
            param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (val));
    };

    setParam ("gateThreshold", p.gateThreshold);
    setParam ("gateDecay",     p.gateDecay);
    setParam ("drive",         p.drive);
    setParam ("tone",          p.tone);
    setParam ("level",         p.level);
    setParam ("mix",           p.mix);
    setParam ("bypass",        p.bypass ? 1.0f : 0.0f);
    setParam ("clipType",      static_cast<float> (p.clipType));
}

void DistortXAudioProcessor::setCurrentProgram (int index)
{
    loadPreset (index);
}

void DistortXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("currentProgram", currentProgram, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DistortXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto vt = juce::ValueTree::fromXml (*xmlState);
        currentProgram = vt.getProperty ("currentProgram", 0);
        apvts.replaceState (vt);
    }
}

// Envuelve el plugin para ser reconocido por los hosts
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortXAudioProcessor();
}