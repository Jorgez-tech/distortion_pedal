#pragma once

#include <JuceHeader.h>

class DistortXAudioProcessor : public juce::AudioProcessor
{
public:
    DistortXAudioProcessor();
    ~DistortXAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Conexión APVTS (Estado de Parámetros)
    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Gain = juce::dsp::Gain<float>;

    struct OversampledClipper
    {
        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
        void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;
        void setClipMode (int newMode) noexcept;

    private:
        std::atomic<int> clipMode { 0 };
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
        juce::dsp::WaveShaper<float> waveshaper;
    };

    using DistortionChain = juce::dsp::ProcessorChain<Filter, Gain, OversampledClipper, Filter, Gain>;

    enum ChainIndex
    {
        preEQIndex = 0,
        preGainIndex,
        clipperIndex,
        postEQIndex,
        outputGainIndex
    };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateHighPassCoefficients() noexcept;
    void updateLowPassCoefficients (float cutoffHz) noexcept;
    static void setBiquadCoefficients (Filter& filter, double sampleRate, float cutoffHz, float q, bool highPass) noexcept;

    DistortionChain dspChain;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    std::atomic<double> currentSampleRate { 44100.0 };
    float lastToneCutoffHz = -1.0f;
    float lastDriveDb = -1000.0f;
    float lastLevelDb = -1000.0f;
    int lastClipMode = -1;
    std::atomic<bool> filtersNeedRefresh { true };

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* toneParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* clipTypeParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXAudioProcessor)
};