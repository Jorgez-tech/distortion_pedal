#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DistortXLookAndFeel.h"

class DistortXAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    DistortXAudioProcessorEditor (DistortXAudioProcessor&);
    ~DistortXAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DistortXAudioProcessor& audioProcessor;
    DistortXLookAndFeel customLookAndFeel;

    // Controles
    juce::Slider gateThresholdSlider;
    juce::Slider gateDecaySlider;
    juce::Slider driveSlider;
    juce::Slider toneSlider;
    juce::Slider levelSlider;
    juce::Slider mixSlider;
    juce::ComboBox clipTypeComboBox;
    juce::ComboBox presetComboBox;
    juce::ToggleButton bypassButton;

    // Etiquetas descriptivas
    juce::Label gateThresholdLabel;
    juce::Label gateDecayLabel;
    juce::Label driveLabel;
    juce::Label toneLabel;
    juce::Label levelLabel;
    juce::Label mixLabel;
    juce::Label clipTypeLabel;
    juce::Label presetLabel;

    // Enlaces APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> clipTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    void drawCornerScrew (juce::Graphics& g, float x, float y);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXAudioProcessorEditor)
};