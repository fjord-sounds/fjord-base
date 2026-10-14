#pragma once
#include "Theme.h"

// Small line icons used across the family (header, preset bar, footers).
namespace fjord::base::ui::icons
{
    // GitHub mark (octicon "mark-github", 16 x 16)
    inline juce::Path githubMark()
    {
        static const juce::Path mark = juce::Drawable::parseSVGPath (
            "M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49"
            "-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52"
            ".28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18"
            " 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87"
            " 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z");
        return mark;
    }

    inline void drawGithub (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        auto mark = githubMark();
        mark.applyTransform (mark.getTransformToScaleToFit (r, true));
        g.setColour (c);
        g.fillPath (mark);
    }

    // Curved arrow for undo (left) / redo (right)
    inline juce::Path undoArrow (juce::Rectangle<float> r, bool redo)
    {
        juce::Path p;
        const auto c = r.getCentre();
        const float rad = r.getWidth() * 0.32f;
        p.addCentredArc (c.x, c.y + 1.5f, rad, rad, 0.0f, -2.2f, 1.2f, true);
        const auto start = c.translated (0, 1.5f).getPointOnCircumference (rad, -2.2f);
        p.startNewSubPath (start.translated (-0.5f, -3.5f));
        p.lineTo (start);
        p.lineTo (start.translated (3.5f, 0.5f));
        if (redo)
            p.applyTransform (juce::AffineTransform::scale (-1.0f, 1.0f, c.x, c.y));
        return p;
    }

    // Theme switch: a sun in the dark theme (switch to light), a moon in the light theme
    inline void drawThemeSwitch (juce::Graphics& g, juce::Rectangle<float> r, bool dark, juce::Colour c)
    {
        g.setColour (c);
        if (dark)
        {
            g.drawEllipse (r.reduced (2.0f), 1.4f);
            for (int i = 0; i < 8; ++i)
            {
                const float a = (float) i * juce::MathConstants<float>::twoPi / 8.0f;
                const auto ctr = r.getCentre();
                g.drawLine ({ ctr.getPointOnCircumference (r.getWidth() * 0.62f, a), ctr.getPointOnCircumference (r.getWidth() * 0.78f, a) }, 1.2f);
            }
        }
        else
        {
            juce::Path moon;
            moon.addEllipse (r);
            juce::Path cut;
            cut.addEllipse (r.translated (r.getWidth() * 0.38f, -r.getHeight() * 0.22f));
            moon.setUsingNonZeroWinding (false);
            moon.addPath (cut);
            g.fillPath (moon);
        }
    }

    inline void drawChevron (juce::Graphics& g, juce::Rectangle<float> area, bool left, juce::Colour c)
    {
        const auto ctr = area.getCentre();
        const float s = 4.0f, dir = left ? 1.0f : -1.0f;
        juce::Path path;
        path.startNewSubPath (ctr.x + s * 0.5f * dir, ctr.y - s);
        path.lineTo (ctr.x - s * 0.5f * dir, ctr.y);
        path.lineTo (ctr.x + s * 0.5f * dir, ctr.y + s);
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Arrow down into a tray – "save"
    inline void drawSave (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour c)
    {
        const auto ctr = area.getCentre();
        juce::Path p;
        p.startNewSubPath (ctr.x, ctr.y - 6.0f);
        p.lineTo (ctr.x, ctr.y + 2.0f);
        p.startNewSubPath (ctr.x - 3.5f, ctr.y - 1.5f);
        p.lineTo (ctr.x, ctr.y + 2.0f);
        p.lineTo (ctr.x + 3.5f, ctr.y - 1.5f);
        p.startNewSubPath (ctr.x - 6.0f, ctr.y + 2.0f);
        p.lineTo (ctr.x - 6.0f, ctr.y + 6.0f);
        p.lineTo (ctr.x + 6.0f, ctr.y + 6.0f);
        p.lineTo (ctr.x + 6.0f, ctr.y + 2.0f);
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}
