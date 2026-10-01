#include "DistortXLookAndFeel.h"

const juce::Colour DistortXLookAndFeel::colourBackground  = juce::Colour::fromRGB (20, 22, 26);
const juce::Colour DistortXLookAndFeel::colourPanel       = juce::Colour::fromRGB (28, 31, 38);
const juce::Colour DistortXLookAndFeel::colourAccentAmber = juce::Colour::fromRGB (245, 166, 35);
const juce::Colour DistortXLookAndFeel::colourAccentRed   = juce::Colour::fromRGB (220, 48, 48);
const juce::Colour DistortXLookAndFeel::colourTextGold    = juce::Colour::fromRGB (230, 200, 140);
const juce::Colour DistortXLookAndFeel::colourTextMuted   = juce::Colour::fromRGB (140, 145, 160);
const juce::Colour DistortXLookAndFeel::colourKnobBody   = juce::Colour::fromRGB (35, 38, 46);
const juce::Colour DistortXLookAndFeel::colourKnobRim    = juce::Colour::fromRGB (60, 65, 78);

DistortXLookAndFeel::DistortXLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, colourTextGold);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (16, 18, 22));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (50, 55, 65));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (24, 27, 34));
    setColour (juce::ComboBox::textColourId, colourTextGold);
    setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (60, 65, 80));
    setColour (juce::ComboBox::arrowColourId, colourAccentAmber);
}

void DistortXLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPosProportional, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                           static_cast<float> (width), static_cast<float> (height));

    // Espacio para el knob (dejando margen para el textbox inferior)
    auto sliderArea = bounds.removeFromTop (bounds.getHeight() - 22.0f).reduced (4.0f);
    const float radius = juce::jmin (sliderArea.getWidth(), sliderArea.getHeight()) * 0.5f;
    const float centreX = sliderArea.getCentreX();
    const float centreY = sliderArea.getCentreY();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Marcas de graduación perimetrales (Ticks)
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float tickAngle = rotaryStartAngle + (static_cast<float> (i) / static_cast<float> (numTicks - 1)) * (rotaryEndAngle - rotaryStartAngle);
        const float cosA = std::cos (tickAngle - juce::MathConstants<float>::halfPi);
        const float sinA = std::sin (tickAngle - juce::MathConstants<float>::halfPi);

        const float innerR = radius + 2.0f;
        const float outerR = radius + (i == 0 || i == numTicks - 1 || i == numTicks / 2 ? 6.0f : 4.0f);

        const bool isActive = (tickAngle <= angle + 0.05f);
        g.setColour (isActive ? colourAccentAmber.withAlpha (0.9f) : colourTextMuted.withAlpha (0.35f));

        g.drawLine (centreX + innerR * cosA, centreY + innerR * sinA,
                    centreX + outerR * cosA, centreY + outerR * sinA,
                    i == 0 || i == numTicks - 1 || i == numTicks / 2 ? 1.8f : 1.2f);
    }

    // 2. Sombra base del potenciómetro
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (centreX - radius + 2.0f, centreY - radius + 3.0f, radius * 2.0f, radius * 2.0f);

    // 3. Bisel exterior metálico (Knurled Rim)
    juce::ColourGradient rimGradient (colourKnobRim.brighter (0.3f), centreX - radius, centreY - radius,
                                      colourKnobRim.darker (0.5f), centreX + radius, centreY + radius, false);
    g.setGradientFill (rimGradient);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // 4. Cuerpo principal del potenciómetro
    const float innerRadius = radius * 0.88f;
    juce::ColourGradient bodyGradient (colourKnobBody.brighter (0.2f), centreX, centreY - innerRadius,
                                       colourKnobBody.darker (0.6f), centreX, centreY + innerRadius, false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // 5. Tapa central biselada con textura radial
    const float capRadius = innerRadius * 0.72f;
    juce::ColourGradient capGradient (juce::Colour::fromRGB (45, 48, 56), centreX - capRadius, centreY - capRadius,
                                      juce::Colour::fromRGB (20, 22, 28), centreX + capRadius, centreY + capRadius, false);
    g.setGradientFill (capGradient);
    g.fillEllipse (centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // 6. Arco activo de nivel
    juce::Path trackPath;
    trackPath.addCentredArc (centreX, centreY, radius * 0.94f, radius * 0.94f,
                             0.0f, rotaryStartAngle, angle, true);
    g.setColour (colourAccentAmber.withAlpha (0.8f));
    g.strokePath (trackPath, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 7. Línea indicadora (Pointer) de alta visibilidad
    const float pointerLength = innerRadius * 0.90f;
    const float pCos = std::cos (angle - juce::MathConstants<float>::halfPi);
    const float pSin = std::sin (angle - juce::MathConstants<float>::halfPi);

    // Resplandor del puntero
    g.setColour (colourAccentAmber.withAlpha (0.35f));
    g.drawLine (centreX, centreY, centreX + pointerLength * pCos, centreY + pointerLength * pSin, 4.5f);

    // Puntero ámbar sólido
    g.setColour (colourAccentAmber);
    g.drawLine (centreX, centreY, centreX + pointerLength * pCos, centreY + pointerLength * pSin, 2.5f);

    // Pequeño pivote central
    g.setColour (juce::Colours::black);
    g.fillEllipse (centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}

void DistortXLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    auto bounds = button.getLocalBounds().toFloat();

    const bool isBypassed = button.getToggleState();
    const bool isActive = !isBypassed;

    // Dibujar footswitch analógico (botón de pie de pedal stompbox)
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY() - 8.0f;
    const float radius = 18.0f;

    // Sombra del footswitch
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (centreX - radius + 1.0f, centreY - radius + 3.0f, radius * 2.0f, radius * 2.0f);

    // Anillo exterior de cromo pulido
    juce::ColourGradient chromeGradient (juce::Colour::fromRGB (180, 185, 195), centreX - radius, centreY - radius,
                                         juce::Colour::fromRGB (70, 75, 85), centreX + radius, centreY + radius, false);
    g.setGradientFill (chromeGradient);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Tuerca hexagonal del footswitch
    const float nutRadius = radius * 0.78f;
    juce::Path nutPath;
    for (int i = 0; i < 6; ++i)
    {
        const float angle = static_cast<float> (i) * juce::MathConstants<float>::pi / 3.0f;
        const float x = centreX + nutRadius * std::cos (angle);
        const float y = centreY + nutRadius * std::sin (angle);
        if (i == 0)
            nutPath.startNewSubPath (x, y);
        else
            nutPath.lineTo (x, y);
    }
    nutPath.closeSubPath();
    g.setColour (juce::Colour::fromRGB (90, 95, 105));
    g.fillPath (nutPath);

    // Botón pulsador central
    const float btnRadius = radius * 0.52f;
    juce::ColourGradient btnGradient (isActive ? juce::Colour::fromRGB (140, 145, 155) : juce::Colour::fromRGB (60, 65, 75),
                                      centreX, centreY - btnRadius,
                                      juce::Colour::fromRGB (30, 32, 38), centreX, centreY + btnRadius, false);
    g.setGradientFill (btnGradient);
    g.fillEllipse (centreX - btnRadius, centreY - btnRadius, btnRadius * 2.0f, btnRadius * 2.0f);

    // Texto de estado
    g.setColour (isActive ? colourTextGold : colourTextMuted);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawFittedText (button.getButtonText(), bounds.removeFromBottom (18).toNearestInt(), juce::Justification::centred, 1);
}

void DistortXLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox& box)
{
    juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
    auto bounds = juce::Rectangle<float> (0, 0, static_cast<float> (width), static_cast<float> (height));

    // Fondo del selector estilo panel industrial
    g.setColour (juce::Colour::fromRGB (22, 24, 30));
    g.fillRoundedRectangle (bounds, 4.0f);

    // Borde metálico
    g.setColour (juce::Colour::fromRGB (60, 65, 80));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.2f);

    // Flecha de menú
    const float arrowX = static_cast<float> (width - 16);
    const float arrowY = static_cast<float> (height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (arrowX - 4.0f, arrowY - 2.5f);
    arrow.lineTo (arrowX + 4.0f, arrowY - 2.5f);
    arrow.lineTo (arrowX, arrowY + 3.0f);
    arrow.closeSubPath();
    g.setColour (colourAccentAmber);
    g.fillPath (arrow);
}

void DistortXLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour::fromRGB (24, 27, 34));
    g.setColour (juce::Colour::fromRGB (60, 65, 80));
    g.drawRect (0, 0, width, height, 1);
}

void DistortXLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool isSeparator, bool isActive, bool isHighlighted,
                                             bool isChecked, bool hasSubMenu, const juce::String& text,
                                             const juce::String& shortcutKeyText, const juce::Drawable* icon,
                                             const juce::Colour* textColour)
{
    juce::ignoreUnused (isSeparator, isActive, isChecked, hasSubMenu, shortcutKeyText, icon, textColour);

    if (isHighlighted)
    {
        g.setColour (colourAccentAmber.withAlpha (0.25f));
        g.fillRect (area);
    }

    g.setColour (isHighlighted ? colourAccentAmber : colourTextGold);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawFittedText (text, area.reduced (10, 0), juce::Justification::centredLeft, 1);
}
