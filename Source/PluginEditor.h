#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DistortXAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    DistortXAudioProcessorEditor (DistortXAudioProcessor&);
    ~DistortXAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DistortXAudioProcessor& audioProcessor;

    juce::Slider driveSlider;
    juce::Slider toneSlider;
    juce::Slider levelSlider;
    juce::Slider mixSlider;
    juce::ComboBox clipTypeComboBox;
    juce::ToggleButton bypassButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> clipTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXAudioProcessorEditor)
};