#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "FjordBaseData.h"

namespace fjord::base::ui
{
    // Manrope (embedded, SIL OFL): Regular for text, Medium for all-caps labels, SemiBold for emphasis.
    enum class Weight { Regular, Medium, SemiBold };

    inline juce::Typeface::Ptr typeface (Weight w)
    {
        static const juce::Typeface::Ptr faces[] = {
            juce::Typeface::createSystemTypefaceFor (FjordBaseData::ManropeRegular_ttf, FjordBaseData::ManropeRegular_ttfSize),
            juce::Typeface::createSystemTypefaceFor (FjordBaseData::ManropeMedium_ttf, FjordBaseData::ManropeMedium_ttfSize),
            juce::Typeface::createSystemTypefaceFor (FjordBaseData::ManropeSemiBold_ttf, FjordBaseData::ManropeSemiBold_ttfSize) };
        return faces[(int) w];
    }

    inline juce::Font font (float height, Weight w)
    {
        return juce::Font (juce::FontOptions().withTypeface (typeface (w)).withHeight (height));
    }

    inline juce::Font font (float height, bool bold = false)
    {
        return font (height, bold ? Weight::SemiBold : Weight::Regular);
    }

    // Small all-caps headings with letter spacing – typical Scandinavian typography.
    inline void drawSpacedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                                float height, juce::Justification just, float kerning = 0.18f, bool bold = false)
    {
        g.setFont (font (height, bold ? Weight::SemiBold : Weight::Medium).withExtraKerningFactor (kerning));
        g.drawText (text, area, just, false);
    }

    // The logo font: lower case, light, a little air between the letters.
    inline juce::Font logoFont (float height) { return font (height).withExtraKerningFactor (0.04f); }
}
