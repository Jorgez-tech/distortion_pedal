#include "PluginProcessor.h"
#include "PluginEditor.h"

DistortXAudioProcessorEditor::DistortXAudioProcessorEditor (DistortXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 300);

    auto configureRotary = [] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    };

    configureRotary (driveSlider);
    configureRotary (toneSlider);
    configureRotary (levelSlider);
    configureRotary (mixSlider);

    clipTypeComboBox.addItem ("Soft", 1);
    clipTypeComboBox.addItem ("Hard", 2);
    bypassButton.setButtonText ("Bypass");

    addAndMakeVisible (driveSlider);
    addAndMakeVisible (toneSlider);
    addAndMakeVisible (levelSlider);
    addAndMakeVisible (mixSlider);
    addAndMakeVisible (clipTypeComboBox);
    addAndMakeVisible (bypassButton);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "drive", driveSlider);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "tone", toneSlider);
    levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "level", levelSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "mix", mixSlider);
    clipTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "clipType", clipTypeComboBox);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "bypass", bypassButton);
}

DistortXAudioProcessorEditor::~DistortXAudioProcessorEditor()
{
}

void DistortXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("DistortX", getLocalBounds().removeFromTop (36), juce::Justification::centred, 1);
}

void DistortXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (36);

    auto topRow = area.removeFromTop (190);
    const int knobSize = 110;
    const int gap = 8;
    driveSlider.setBounds (topRow.removeFromLeft (knobSize));
    topRow.removeFromLeft (gap);
    toneSlider.setBounds (topRow.removeFromLeft (knobSize));
    topRow.removeFromLeft (gap);
    levelSlider.setBounds (topRow.removeFromLeft (knobSize));
    topRow.removeFromLeft (gap);
    mixSlider.setBounds (topRow.removeFromLeft (knobSize));

    auto bottomRow = area.removeFromTop (40);
    clipTypeComboBox.setBounds (bottomRow.removeFromLeft (140));
    bottomRow.removeFromLeft (12);
    bypassButton.setBounds (bottomRow.removeFromLeft (120));
}