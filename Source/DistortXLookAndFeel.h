#pragma once

#include <JuceHeader.h>

class DistortXLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DistortXLookAndFeel();
    ~DistortXLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isChecked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon,
                            const juce::Colour* textColour) override;

    // Paleta de colores analógica
    static const juce::Colour colourBackground;
    static const juce::Colour colourPanel;
    static const juce::Colour colourAccentAmber;
    static const juce::Colour colourAccentRed;
    static const juce::Colour colourTextGold;
    static const juce::Colour colourTextMuted;
    static const juce::Colour colourKnobBody;
    static const juce::Colour colourKnobRim;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortXLookAndFeel)
};
