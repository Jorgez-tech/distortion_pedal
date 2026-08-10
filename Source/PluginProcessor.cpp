#include "PluginProcessor.h"
#include "PluginEditor.h"

void DistortXAudioProcessor::OversampledClipper::prepare (const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->reset();
    oversampling->initProcessing (spec.maximumBlockSize);

    juce::dsp::ProcessSpec upsampledSpec { spec.sampleRate * oversampling->getOversamplingFactor(),
                                           static_cast<juce::uint32> (spec.maximumBlockSize * oversampling->getOversamplingFactor()),
                                           spec.numChannels };
    setClipMode (clipMode.load (std::memory_order_relaxed));
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

void DistortXAudioProcessor::OversampledClipper::setClipMode (int newMode) noexcept
{
    clipMode.store (newMode, std::memory_order_relaxed);

    if (newMode == 0)
    {
        waveshaper.functionToUse = [] (float x) noexcept
        {
            return std::tanh (x);
        };

        return;
    }

    waveshaper.functionToUse = [] (float x) noexcept
    {
        return juce::jlimit (-1.0f, 1.0f, x);
    };
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

    dspChain.prepare (spec);
    dspChain.reset();
    dspChain.get<gateIndex>().setRatio (100.0f);
    dspChain.get<gateIndex>().setAttack (2.0f);
    dspChain.get<gateIndex>().setThreshold (juce::jlimit (-100.0f, 0.0f, gateThresholdParam->load (std::memory_order_relaxed)));
    dspChain.get<gateIndex>().setRelease (juce::jlimit (5.0f, 500.0f, gateDecayParam->load (std::memory_order_relaxed)));
    dspChain.get<preGainIndex>().setRampDurationSeconds (0.02);
    dspChain.get<outputGainIndex>().setRampDurationSeconds (0.02);

    mixSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, mixParam->load (std::memory_order_relaxed)));

    dryBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);

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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (bypassParam->load (std::memory_order_relaxed) > 0.5f)
        return;

    const int numSamples = buffer.getNumSamples();
    if (totalNumInputChannels > dryBuffer.getNumChannels() || numSamples > dryBuffer.getNumSamples())
    {
        jassertfalse;
        return;
    }

    jassert (totalNumInputChannels <= 2);

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

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    dspChain.get<gateIndex>().process (context);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        juce::FloatVectorOperations::copy (dryBuffer.getWritePointer (channel), buffer.getReadPointer (channel), numSamples);

    dspChain.get<preEQIndex>().process (context);
    dspChain.get<preGainIndex>().process (context);
    dspChain.get<clipperIndex>().process (context);
    dspChain.get<postEQIndex>().process (context);
    dspChain.get<outputGainIndex>().process (context);

    float* wetPointers[2] {};
    const float* dryPointers[2] {};
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        wetPointers[channel] = buffer.getWritePointer (channel);
        dryPointers[channel] = dryBuffer.getReadPointer (channel);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto mix = mixSmoothed.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            wetPointers[channel][sample] = juce::jmap (mix, dryPointers[channel][sample], wetPointers[channel][sample]);
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

    if (highPass)
    {
        *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, clampedCutoff, q);
        return;
    }

    *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, clampedCutoff, q);
}

bool DistortXAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DistortXAudioProcessor::createEditor()
{
    // Usaremos un Editor genérico temporalmente o directamente el nuestro
    return new DistortXAudioProcessorEditor (*this);
}

void DistortXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serializar el estado hacia Reaper
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DistortXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Cargar el estado desde Reaper
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// Envuelve el plugin para ser reconocido por los hosts
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortXAudioProcessor();
}