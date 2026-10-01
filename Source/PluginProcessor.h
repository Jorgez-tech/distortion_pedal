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

    struct Preset
    {
        const char* name;
        float gateThreshold; // dB
        float gateDecay;     // ms
        float drive;         // dB
        float tone;          // Hz
        float level;         // dB
        float mix;           // 0..1
        bool bypass;
        int clipType;        // 0 = Soft, 1 = Hard
    };

    static const std::vector<Preset>& getFactoryPresets();
    void loadPreset (int presetIndex);

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Conexión APVTS (Estado de Parámetros)
    juce::AudioProcessorValueTreeState apvts;

private:
    using NoiseGate = juce::dsp::NoiseGate<float>;
    using Filter    = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain      = juce::dsp::Gain<float>;

    //==============================================================================
    // OversampledClipper
    // Encapsula el Oversampling x4 + WaveShaper.
    // setClipMode() es llamado EXCLUSIVAMENTE desde el audio thread (processBlock),
    // por lo que la actualización del waveshaper es segura sin locks adicionales.
    // El modo se lee como un entero atómico y el WaveShaper se reconfigura solo
    // cuando cambia, evitando asignaciones en el hot path.
    //==============================================================================
    struct OversampledClipper
    {
        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
        void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;
        // Debe llamarse únicamente desde el audio thread.
        void setClipMode (int newMode) noexcept;

        float getLatencyInSamples() const noexcept
        {
            return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
        }

    private:
        int currentMode = -1;   // cacheado en el audio thread — sin atomic necesario
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
        juce::dsp::WaveShaper<float> waveshaper;
    };

    //==============================================================================
    // Cadena DSP principal:
    // NoiseGate -> Pre-EQ (HPF @120Hz) -> Drive Gain -> OversampledClipper
    //          -> Post-EQ (LPF Tone) -> Level Gain
    //==============================================================================
    using DistortionChain = juce::dsp::ProcessorChain<NoiseGate, Filter, Gain,
                                                       OversampledClipper, Filter, Gain>;

    enum ChainIndex
    {
        gateIndex = 0,
        preEQIndex,
        preGainIndex,
        clipperIndex,
        postEQIndex,
        outputGainIndex
    };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Helpers de coeficientes — llamados desde el audio thread con cero reservas dinámicas (Zero-Allocation)
    void updateHighPassCoefficients() noexcept;
    void updateLowPassCoefficients (float cutoffHz) noexcept;
    static void setBiquadCoefficients (Filter& filter, double sampleRate,
                                       float cutoffHz, float q, bool highPass) noexcept;

    DistortionChain dspChain;
    // Capacidad fija de 1024 muestras (DSP-01):
    // La latencia del filtro IIR polyphase x4 a 192 kHz es ~24 muestras como máximo.
    // 1024 ofrece margen x40 — invariante seguro para cualquier SR soportado por VST3.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dryDelayLine { 1024 };
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> rawInputBuffer;

    // SmoothedValue para Dry/Wet y Bypass — Linear para crossfade sin coloración ni zipper noise
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoothed;

    // Sample rate cacheado para helpers de filtro (puede leerse desde el audio thread)
    std::atomic<double> currentSampleRate { 44100.0 };

    // Valores cacheados para evitar actualizaciones redundantes en el audio thread
    float lastToneCutoffHz      = -1.0f;
    float lastDriveDb           = -1000.0f;
    float lastLevelDb           = -1000.0f;
    float lastGateThresholdDb   = -1000.0f;
    float lastGateDecayMs       = -1.0f;
    int   lastClipMode          = -1;
    int   currentProgram        = 0;

    // Flag cross-thread: el editor/host solicita recalcular coeficientes
    // (p.ej. cambio de sample rate en prepareToPlay desde el message thread)
    std::atomic<bool> filtersNeedRefresh { true };

    // Punteros directos a los parámetros APVTS — lock-free, recomendados por JUCE
    std::atomic<float>* gateThresholdParam = nullptr;
    std::atomic<float>* gateDecayParam     = nullptr;
    std::atomic<float>* driveParam         = nullptr;
    std::atomic<float>* toneParam          = nullptr;
    std::atomic<float>* levelParam         = nullptr;
    std::atomic<float>* mixParam           = nullptr;
    std::atomic<float>* bypassParam        = nullptr;
    std::atomic<float>* clipTypeParam      = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXAudioProcessor)
};