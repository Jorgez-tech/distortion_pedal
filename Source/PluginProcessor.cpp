#include "PluginProcessor.h"
#include "PluginEditor.h"

void DistortXAudioProcessor::OversampledClipper::prepare (const juce::dsp::ProcessSpec& spec)
{
    waveshaper.functionToUse = [this] (float x) noexcept
    {
        if (clipMode.load (std::memory_order_relaxed) == 0)
            return std::tanh (x);

        return juce::jlimit (-1.0f, 1.0f, x);
    };

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->reset();
    oversampling->initProcessing (spec.maximumBlockSize);

    juce::dsp::ProcessSpec upsampledSpec { spec.sampleRate * oversampling->getOversamplingFactor(),
                                           spec.maximumBlockSize * oversampling->getOversamplingFactor(),
                                           spec.numChannels };
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
    dspChain.get<preGainIndex>().setRampDurationSeconds (0.02);
    dspChain.get<outputGainIndex>().setRampDurationSeconds (0.02);

    mixSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, mixParam->load (std::memory_order_relaxed)));

    dryBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);

    lastToneCutoffHz = -1.0f;
    lastDriveDb = -1000.0f;
    lastLevelDb = -1000.0f;
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

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        juce::FloatVectorOperations::copy (dryBuffer.getWritePointer (channel), buffer.getReadPointer (channel), numSamples);

    const auto driveDb = driveParam->load (std::memory_order_relaxed);
    const auto levelDb = levelParam->load (std::memory_order_relaxed);
    const auto mixValue = juce::jlimit (0.0f, 1.0f, mixParam->load (std::memory_order_relaxed));
    const auto toneCutoff = juce::jlimit (800.0f, 18000.0f, toneParam->load (std::memory_order_relaxed));

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
    dspChain.process (context);

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
    const auto omega = juce::MathConstants<float>::twoPi * clampedCutoff / static_cast<float> (sampleRate);
    const auto sinOmega = std::sin (omega);
    const auto cosOmega = std::cos (omega);
    const auto alpha = sinOmega / (2.0f * q);

    float b0 = 0.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;

    if (highPass)
    {
        b0 = (1.0f + cosOmega) * 0.5f;
        b1 = -(1.0f + cosOmega);
        b2 = b0;
    }
    else
    {
        b0 = (1.0f - cosOmega) * 0.5f;
        b1 = 1.0f - cosOmega;
        b2 = b0;
    }

    const auto a0 = 1.0f + alpha;
    const auto a1 = -2.0f * cosOmega;
    const auto a2 = 1.0f - alpha;
    const auto invA0 = 1.0f / a0;

    auto& c = filter.coefficients->coefficients;
    c[0] = b0 * invA0;
    c[1] = b1 * invA0;
    c[2] = b2 * invA0;
    c[3] = a1 * invA0;
    c[4] = a2 * invA0;
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