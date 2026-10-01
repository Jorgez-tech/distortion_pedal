#include "PluginProcessor.h"
#include "PluginEditor.h"

DistortXAudioProcessorEditor::DistortXAudioProcessorEditor (DistortXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);
    setSize (780, 360);

    auto configureRotary = [this] (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setFont (juce::Font (11.5f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, DistortXLookAndFeel::colourTextGold);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    configureRotary (gateThresholdSlider, gateThresholdLabel, "THRESHOLD");
    configureRotary (gateDecaySlider, gateDecayLabel, "GATE DECAY");
    configureRotary (driveSlider, driveLabel, "DRIVE");
    configureRotary (toneSlider, toneLabel, "TONE");
    configureRotary (levelSlider, levelLabel, "LEVEL");
    configureRotary (mixSlider, mixLabel, "MIX");

    clipTypeComboBox.addItem ("Soft (Tube)", 1);
    clipTypeComboBox.addItem ("Hard (Diode)", 2);
    addAndMakeVisible (clipTypeComboBox);

    clipTypeLabel.setText ("CLIP MODE", juce::dontSendNotification);
    clipTypeLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    clipTypeLabel.setColour (juce::Label::textColourId, DistortXLookAndFeel::colourTextGold);
    clipTypeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (clipTypeLabel);

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, DistortXLookAndFeel::colourTextGold);
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);

    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetComboBox.addItem (audioProcessor.getProgramName (i), i + 1);

    presetComboBox.setSelectedId (audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetComboBox.onChange = [this]
    {
        const int selectedIdx = presetComboBox.getSelectedId() - 1;
        if (selectedIdx >= 0)
            audioProcessor.setCurrentProgram (selectedIdx);
    };
    addAndMakeVisible (presetComboBox);

    bypassButton.setButtonText ("STOMP BYPASS");
    addAndMakeVisible (bypassButton);

    // Conexiones bidireccionales con APVTS
    gateThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "gateThreshold", gateThresholdSlider);
    gateDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "gateDecay", gateDecaySlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "drive", driveSlider);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "tone", toneSlider);
    levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "level", levelSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "mix", mixSlider);
    clipTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "clipType", clipTypeComboBox);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "bypass", bypassButton);
}

DistortXAudioProcessorEditor::~DistortXAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void DistortXAudioProcessorEditor::drawCornerScrew (juce::Graphics& g, float x, float y)
{
    const float radius = 5.0f;
    juce::ColourGradient screwGradient (juce::Colour::fromRGB (160, 165, 175), x - radius, y - radius,
                                        juce::Colour::fromRGB (50, 55, 65), x + radius, y + radius, false);
    g.setGradientFill (screwGradient);
    g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);

    // Ranura del tornillo
    g.setColour (juce::Colour::fromRGB (25, 28, 35));
    g.drawLine (x - 3.0f, y - 1.0f, x + 3.0f, y + 1.0f, 1.2f);
}

void DistortXAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto totalArea = getLocalBounds().toFloat();

    // 1. Chasis exterior metálico (Stompbox Enclosure)
    juce::ColourGradient bgGradient (DistortXLookAndFeel::colourBackground.brighter (0.1f), 0, 0,
                                     DistortXLookAndFeel::colourBackground.darker (0.3f), 0, totalArea.getHeight(), false);
    g.setGradientFill (bgGradient);
    g.fillRoundedRectangle (totalArea, 8.0f);

    // Bisel exterior perimetral
    g.setColour (juce::Colour::fromRGB (65, 70, 82));
    g.drawRoundedRectangle (totalArea.reduced (1.0f), 8.0f, 1.5f);

    // 2. Tornillos en las cuatro esquinas
    drawCornerScrew (g, 16.0f, 16.0f);
    drawCornerScrew (g, totalArea.getWidth() - 16.0f, 16.0f);
    drawCornerScrew (g, 16.0f, totalArea.getHeight() - 16.0f);
    drawCornerScrew (g, totalArea.getWidth() - 16.0f, totalArea.getHeight() - 16.0f);

    // Altura del header: constante compartida con resized() para evitar desalineaciones (UI-01).
    static constexpr float kHeaderHeight = 52.0f;

    // 3. Encabezado / Placa de marca (Header Plate)
    auto headerArea = totalArea.removeFromTop (kHeaderHeight);
    g.setColour (DistortXLookAndFeel::colourPanel.darker (0.2f));
    g.fillRect (headerArea.reduced (12.0f, 4.0f));

    g.setColour (DistortXLookAndFeel::colourTextGold);
    g.setFont (juce::Font (21.0f, juce::Font::bold));
    g.drawText ("DISTORTX", headerArea.removeFromLeft (130.0f).translated (24.0f, 0.0f), juce::Justification::centredLeft);

    g.setColour (DistortXLookAndFeel::colourTextMuted);
    g.setFont (juce::Font (9.5f, juce::Font::plain));
    g.drawText ("ANALOG SATURATION ENGINE // VST3", headerArea.removeFromLeft (260.0f).translated (10.0f, 0.0f), juce::Justification::centredLeft);

    // 4. Indicador LED Joya (Status LED)
    const bool isBypassed = bypassButton.getToggleState();
    const bool isEngaged = !isBypassed;
    const float ledX = totalArea.getWidth() - 60.0f;
    const float ledY = 26.0f;
    const float ledRadius = 7.0f;

    // Sombra del LED
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillEllipse (ledX - ledRadius - 2.0f, ledY - ledRadius - 2.0f, (ledRadius + 2.0f) * 2.0f, (ledRadius + 2.0f) * 2.0f);

    // Bisel cromado del LED
    g.setColour (juce::Colour::fromRGB (160, 165, 175));
    g.drawEllipse (ledX - ledRadius, ledY - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f, 1.5f);

    if (isEngaged)
    {
        // Resplandor del LED activo
        juce::ColourGradient glow (DistortXLookAndFeel::colourAccentRed.withAlpha (0.6f), ledX, ledY,
                                   DistortXLookAndFeel::colourAccentRed.withAlpha (0.0f), ledX + 22.0f, ledY + 22.0f, true);
        g.setGradientFill (glow);
        g.fillEllipse (ledX - 22.0f, ledY - 22.0f, 44.0f, 44.0f);

        // Lente brillante
        juce::ColourGradient lens (juce::Colour::fromRGB (255, 120, 120), ledX - 2.0f, ledY - 2.0f,
                                   DistortXLookAndFeel::colourAccentRed.darker (0.2f), ledX + 4.0f, ledY + 4.0f, true);
        g.setGradientFill (lens);
        g.fillEllipse (ledX - ledRadius + 1.0f, ledY - ledRadius + 1.0f, (ledRadius - 1.0f) * 2.0f, (ledRadius - 1.0f) * 2.0f);

        // Brillo especular
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (ledX - 3.0f, ledY - 4.0f, 3.0f, 2.0f);
    }
    else
    {
        // LED apagado
        g.setColour (juce::Colour::fromRGB (50, 15, 15));
        g.fillEllipse (ledX - ledRadius + 1.0f, ledY - ledRadius + 1.0f, (ledRadius - 1.0f) * 2.0f, (ledRadius - 1.0f) * 2.0f);
    }

    // 5. Paneles y Líneas divisorias de sección
    auto mainArea = getLocalBounds().reduced (14).toFloat();
    mainArea.removeFromTop (44.0f);

    // Separador vertical entre Gate (izq) y Drive/Tone (der)
    const float splitX = mainArea.getX() + 230.0f;
    g.setColour (juce::Colour::fromRGB (45, 48, 60));
    g.drawVerticalLine (static_cast<int> (splitX), mainArea.getY() + 10.0f, mainArea.getBottom() - 10.0f);

    // Títulos de sección estilo serigrafía
    g.setColour (DistortXLookAndFeel::colourTextMuted.withAlpha (0.8f));
    g.setFont (juce::Font (9.5f, juce::Font::bold));
    g.drawText ("NOISE GATE", static_cast<int> (mainArea.getX() + 10.0f), static_cast<int> (mainArea.getY() + 2.0f), 200, 16, juce::Justification::centred);
    g.drawText ("DRIVE & TONE ENGINE", static_cast<int> (splitX + 10.0f), static_cast<int> (mainArea.getY() + 2.0f), 500, 16, juce::Justification::centred);
}

void DistortXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    // kHeaderHeight debe coincidir con el valor usado en paint() (UI-01 resuelto).
    static constexpr int kHeaderHeight = 52;

    // Selector de Presets en la cabecera
    presetLabel.setBounds (445, 14, 60, 24);
    presetComboBox.setBounds (510, 14, 185, 24);

    area.removeFromTop (kHeaderHeight); // Espacio para cabecera

    // Fila superior: Perillas
    auto knobsRow = area.removeFromTop (200);

    const int knobWidth = 105;
    const int gap = 6;
    const int labelHeight = 16;

    // Sección Gate (Izquierda)
    auto gate1Area = knobsRow.removeFromLeft (knobWidth);
    gateThresholdLabel.setBounds (gate1Area.removeFromTop (labelHeight));
    gateThresholdSlider.setBounds (gate1Area);

    knobsRow.removeFromLeft (gap);

    auto gate2Area = knobsRow.removeFromLeft (knobWidth);
    gateDecayLabel.setBounds (gate2Area.removeFromTop (labelHeight));
    gateDecaySlider.setBounds (gate2Area);

    knobsRow.removeFromLeft (gap + 16); // Espacio para la línea divisoria

    // Sección Drive & Tone (Derecha)
    auto driveArea = knobsRow.removeFromLeft (knobWidth);
    driveLabel.setBounds (driveArea.removeFromTop (labelHeight));
    driveSlider.setBounds (driveArea);

    knobsRow.removeFromLeft (gap);

    auto toneArea = knobsRow.removeFromLeft (knobWidth);
    toneLabel.setBounds (toneArea.removeFromTop (labelHeight));
    toneSlider.setBounds (toneArea);

    knobsRow.removeFromLeft (gap);

    auto levelArea = knobsRow.removeFromLeft (knobWidth);
    levelLabel.setBounds (levelArea.removeFromTop (labelHeight));
    levelSlider.setBounds (levelArea);

    knobsRow.removeFromLeft (gap);

    auto mixArea = knobsRow.removeFromLeft (knobWidth);
    mixLabel.setBounds (mixArea.removeFromTop (labelHeight));
    mixSlider.setBounds (mixArea);

    // Fila inferior: Controles maestros (Clip Mode y Bypass)
    auto footerRow = area.removeFromTop (56);
    footerRow.removeFromLeft (20);

    clipTypeLabel.setBounds (footerRow.removeFromLeft (80).withHeight (28).translated (0, 4));
    clipTypeComboBox.setBounds (footerRow.removeFromLeft (140).withHeight (28).translated (0, 4));

    footerRow.removeFromLeft (160);

    bypassButton.setBounds (footerRow.removeFromLeft (180).withHeight (48));
}