#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "../Source/PluginProcessor.h"

TEST_CASE ("Filter Stability: HPF 120 Hz DC Attenuation", "[dsp][filter]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    processor.prepareToPlay (sampleRate, blockSize);

    // Configurar Drive en 0 dB, Mix en 100%, Gate en -100 dB (abierto)
    auto* driveParam = processor.apvts.getRawParameterValue ("drive");
    auto* mixParam = processor.apvts.getRawParameterValue ("mix");
    auto* gateThresh = processor.apvts.getRawParameterValue ("gateThreshold");
    
    if (driveParam) driveParam->store (0.0f);
    if (mixParam) mixParam->store (1.0f);
    if (gateThresh) gateThresh->store (-100.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    // Llenar con señal DC constante (amplitud 0.5)
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.5f);

    // Procesar varios bloques para que el filtro HPF alcance estado estacionario
    for (int block = 0; block < 10; ++block)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, 0.5f);

        processor.processBlock (buffer, midi);
    }

    // Comprobar que la señal DC continua ha sido fuertemente atenuada por el HPF a 120 Hz
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = blockSize / 2; i < blockSize; ++i)
        {
            float sample = buffer.getSample (ch, i);
            REQUIRE (std::isfinite (sample));
            REQUIRE (std::abs (sample) < 0.05f); // Gran atenuación de DC
        }
    }
}

TEST_CASE ("Filter Stability: LPF Tone Sweep without NaN or Inf", "[dsp][filter]")
{
    DistortXAudioProcessor processor;
    const double sampleRate = 44100.0;
    const int blockSize = 256;
    processor.prepareToPlay (sampleRate, blockSize);

    auto* toneParam = processor.apvts.getRawParameterValue ("tone");
    auto* gateThresh = processor.apvts.getRawParameterValue ("gateThreshold");
    if (gateThresh) gateThresh->store (-100.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    const float testFrequencies[] = { 800.0f, 1500.0f, 3500.0f, 6500.0f, 10000.0f, 14000.0f, 18000.0f };

    for (float cutoff : testFrequencies)
    {
        if (toneParam) toneParam->store (cutoff);

        // Llenar con ruido blanco acotado
        juce::Random rng (12345);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, (rng.nextFloat() * 2.0f - 1.0f) * 0.25f);

        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                float s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
                REQUIRE_FALSE (std::isnan (s));
                REQUIRE (std::abs (s) <= 2.0f);
            }
        }
    }
}

TEST_CASE ("Filter Stability: Multi Sample Rate Support", "[dsp][samplerate]")
{
    DistortXAudioProcessor processor;
    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const int blockSize = 128;

    for (double sr : rates)
    {
        processor.prepareToPlay (sr, blockSize);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample (ch, i, std::sin (2.0f * juce::MathConstants<float>::pi * 440.0f * (float) i / (float) sr) * 0.3f);

        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                float s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
            }
        }
    }
}
