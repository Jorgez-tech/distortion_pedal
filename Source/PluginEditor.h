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
    
    // UI temporal
    juce::Slider driveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXAudioProcessorEditor)
};