#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "../Source/PluginProcessor.h"

TEST_CASE ("Latency & PDC: Latency Reporting", "[dsp][latency][pdc]")
{
    DistortXAudioProcessor processor;
    processor.prepareToPlay (44100.0, 256);

    const int latencySamples = processor.getLatencySamples();

    // El sobremuestreo x4 polyphase IIR introduce una pequeña latencia
    REQUIRE (latencySamples >= 0);
    REQUIRE (latencySamples <= 1024); // Capacidad del dryDelayLine
}

TEST_CASE ("Latency & PDC: Dry/Wet Phase Coherence", "[dsp][latency][mix]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    processor.prepareToPlay (sampleRate, blockSize);

    auto* driveParam = processor.apvts.getRawParameterValue ("drive");
    auto* mixParam = processor.apvts.getRawParameterValue ("mix");
    auto* toneParam = processor.apvts.getRawParameterValue ("tone");
    auto* gateThresh = processor.apvts.getRawParameterValue ("gateThreshold");

    // Ganancia unitaria, tono abierto, gate abierto
    if (driveParam) driveParam->store (0.0f);
    if (toneParam) toneParam->store (18000.0f);
    if (gateThresh) gateThresh->store (-100.0f);

    // 1. Probar a Mix = 0.5 (50% Dry, 50% Wet)
    if (mixParam) mixParam->store (0.5f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    // Inyectar seno a 1000 Hz de pequeña amplitud (régimen lineal)
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.1f * std::sin (2.0f * juce::MathConstants<float>::pi * 1000.0f * (float) i / (float) sampleRate));

    // Dejar que el procesador y el delay line se estabilicen
    for (int block = 0; block < 5; ++block)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, 0.1f * std::sin (2.0f * juce::MathConstants<float>::pi * 1000.0f * (float) i / (float) sampleRate));

        processor.processBlock (buffer, midi);
    }

    // La señal no debe cancelarse a cero debido a desfase
    float maxAmp = 0.0f;
    for (int i = blockSize / 2; i < blockSize; ++i)
        maxAmp = std::max (maxAmp, std::abs (buffer.getSample (0, i)));

    REQUIRE (maxAmp > 0.03f); // Suma constructiva mantenida
}
