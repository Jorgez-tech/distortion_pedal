#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "../Source/PluginProcessor.h"

TEST_CASE ("Realtime Stress: Extreme Buffer Sizes", "[dsp][stress]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int bufferSizes[] = { 32, 64, 128, 256, 512, 1024, 2048 };

    for (int size : bufferSizes)
    {
        processor.prepareToPlay (sampleRate, size);

        juce::AudioBuffer<float> buffer (2, size);
        juce::MidiBuffer midi;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < size; ++i)
                buffer.setSample (ch, i, 0.2f * std::sin (2.0f * juce::MathConstants<float>::pi * 440.0f * (float) i / (float) sampleRate));

        REQUIRE_NOTHROW (processor.processBlock (buffer, midi));

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < size; ++i)
            {
                float s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
            }
        }
    }
}

TEST_CASE ("Realtime Stress: Soft-Bypass Ramping", "[dsp][bypass]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 256;
    processor.prepareToPlay (sampleRate, blockSize);

    auto* bypassParam = processor.apvts.getRawParameterValue ("bypass");
    auto* driveParam  = processor.apvts.getRawParameterValue ("drive");
    if (driveParam) driveParam->store (18.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    // 1. Procesar con plugin activo
    if (bypassParam) bypassParam->store (0.0f);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.4f);

    processor.processBlock (buffer, midi);

    // 2. Activar Bypass a mitad de la reproducción
    if (bypassParam) bypassParam->store (1.0f);
    processor.processBlock (buffer, midi);

    // Comprobar que no hay NaN o discontinuidades infinitas
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            float s = buffer.getSample (ch, i);
            REQUIRE (std::isfinite (s));
        }
    }
}

TEST_CASE ("Realtime Stress: Noise Gate Attenuation", "[dsp][gate]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    processor.prepareToPlay (sampleRate, blockSize);

    auto* gateThresh = processor.apvts.getRawParameterValue ("gateThreshold");
    auto* gateDecay  = processor.apvts.getRawParameterValue ("gateDecay");
    auto* driveParam = processor.apvts.getRawParameterValue ("drive");

    if (gateThresh) gateThresh->store (-40.0f); // Umbral a -40 dB
    if (gateDecay)  gateDecay->store (10.0f);
    if (driveParam) driveParam->store (0.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    // Señal por debajo del umbral (-60 dBFS = aprox. 0.001)
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.001f);

    // Procesar varios bloques para que el gate cierre
    for (int block = 0; block < 10; ++block)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, 0.001f);

        processor.processBlock (buffer, midi);
    }

    // La señal resultante debe haber sido atenuada por el Gate
    float maxLevel = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            maxLevel = std::max (maxLevel, std::abs (buffer.getSample (ch, i)));

    REQUIRE (maxLevel < 0.0005f);
}
