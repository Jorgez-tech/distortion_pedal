#include "PluginProcessor.h"
#include "PluginEditor.h"

DistortXAudioProcessorEditor::DistortXAudioProcessorEditor (DistortXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Configuración mínima de la ventana principal
    setSize (400, 300);

    // Configuración de nuestro knob de prueba
    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(driveSlider);

    // Conectamos la interfaz (UI) con el algoritmo de DSP a través del APVTS
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "drive", driveSlider);
}

DistortXAudioProcessorEditor::~DistortXAudioProcessorEditor()
{
}

void DistortXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("DistortX - Phase 1", getLocalBounds().withSizeKeepingCentre(200, 50).translated(0, -100), juce::Justification::centred, 1);
}

void DistortXAudioProcessorEditor::resized()
{
    driveSlider.setBounds(150, 100, 100, 100);
}