#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    
    // Aquí implementaremos Drive, Tone, Level, Mix, Bypass, Type
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"drive", 1}, "Drive", 1.0f, 10.0f, 1.0f));

    return { params.begin(), params.end() };
}

void DistortXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Aquí inicializaremos Oversampling, Filtros, SmoothedValues
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Loop estricto de DSP en tiempo real sin bloqueos
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // Bloque listo para recibir la matemática de saturación (Fase 1-2)
    }
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