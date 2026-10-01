#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Source/PluginProcessor.h"

using Catch::Matchers::WithinAbs;

TEST_CASE ("APVTS State: Parameter Existence and Initial Layout", "[apvts][parameters]")
{
    DistortXAudioProcessor processor;

    const juce::StringArray expectedParamIDs = {
        "gateThreshold",
        "gateDecay",
        "drive",
        "tone",
        "level",
        "mix",
        "bypass",
        "clipType"
    };

    for (const auto& paramID : expectedParamIDs)
    {
        auto* param = processor.apvts.getParameter (paramID);
        REQUIRE (param != nullptr);
    }
}

TEST_CASE ("APVTS State: XML Serialization and Deserialization", "[apvts][state]")
{
    DistortXAudioProcessor processor1;
    DistortXAudioProcessor processor2;

    // Helper para asignar valor en espacio real notificando al APVTS ValueTree
    auto setParamValue = [&] (const juce::String& paramID, float plainValue)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (processor1.apvts.getParameter (paramID)))
            p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (plainValue));
    };

    setParamValue ("drive", 28.5f);
    setParamValue ("tone", 12500.0f);
    setParamValue ("gateThreshold", -45.0f);
    setParamValue ("mix", 0.75f);
    setParamValue ("clipType", 1.0f); // Hard

    // Serializar estado a MemoryBlock
    juce::MemoryBlock stateBlock;
    processor1.getStateInformation (stateBlock);
    REQUIRE (stateBlock.getSize() > 0);

    // Restaurar en processor2
    processor2.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));

    auto* drive2 = processor2.apvts.getRawParameterValue ("drive");
    auto* tone2  = processor2.apvts.getRawParameterValue ("tone");
    auto* gateT2 = processor2.apvts.getRawParameterValue ("gateThreshold");
    auto* mix2   = processor2.apvts.getRawParameterValue ("mix");
    auto* clip2  = processor2.apvts.getRawParameterValue ("clipType");

    REQUIRE (drive2 != nullptr);
    REQUIRE (tone2 != nullptr);
    REQUIRE (gateT2 != nullptr);
    REQUIRE (mix2 != nullptr);
    REQUIRE (clip2 != nullptr);

    REQUIRE_THAT (drive2->load(), WithinAbs (28.5f, 0.05f));
    REQUIRE_THAT (tone2->load(),  WithinAbs (12500.0f, 1.0f));
    REQUIRE_THAT (gateT2->load(), WithinAbs (-45.0f, 0.1f));
    REQUIRE_THAT (mix2->load(),   WithinAbs (0.75f, 0.01f));
    REQUIRE (static_cast<int> (clip2->load()) == 1);
}

TEST_CASE ("Factory Presets: Bank Verification and Loading", "[presets]")
{
    DistortXAudioProcessor processor;

    const int numPrograms = processor.getNumPrograms();
    REQUIRE (numPrograms >= 6);

    for (int i = 0; i < numPrograms; ++i)
    {
        juce::String name = processor.getProgramName (i);
        REQUIRE (name.isNotEmpty());

        processor.setCurrentProgram (i);
        REQUIRE (processor.getCurrentProgram() == i);

        // Validar que los parámetros tras cargar el preset están dentro de rangos válidos
        auto* drive = processor.apvts.getRawParameterValue ("drive");
        auto* tone  = processor.apvts.getRawParameterValue ("tone");
        auto* gateT = processor.apvts.getRawParameterValue ("gateThreshold");
        auto* mix   = processor.apvts.getRawParameterValue ("mix");

        REQUIRE (drive != nullptr);
        REQUIRE (tone != nullptr);
        REQUIRE (gateT != nullptr);
        REQUIRE (mix != nullptr);

        REQUIRE (drive->load() >= 0.0f);
        REQUIRE (drive->load() <= 36.0f);
        REQUIRE (tone->load() >= 800.0f);
        REQUIRE (tone->load() <= 18000.0f);
        REQUIRE (gateT->load() >= -100.0f);
        REQUIRE (gateT->load() <= 0.0f);
        REQUIRE (mix->load() >= 0.0f);
        REQUIRE (mix->load() <= 1.0f);
    }
}
