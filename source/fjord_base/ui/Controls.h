#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "../presets/Locks.h"

namespace fjord::base::ui
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // Slider that leaves horizontal wheel/trackpad swipes to its parents, so scrollable strips keep scrolling over knobs.
    class KnobSlider : public juce::Slider
    {
    public:
        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
        {
            if (std::abs (w.deltaX) > std::abs (w.deltaY))
                juce::Component::mouseWheelMove (e, w);   // passes the event on to the parent
            else
                juce::Slider::mouseWheelMove (e, w);
        }
    };

    // Rotary knob with its name below; hovering or dragging shows the value.
    class Knob : public juce::Component
    {
    public:
        Knob (APVTS& state, const juce::String& paramId, const juce::String& labelText)
            : name (labelText)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
            slider.setMouseDragSensitivity (220);
            slider.setVelocityBasedMode (false);
            addAndMakeVisible (slider);

            auto* param = state.getParameter (paramId);
            jassert (param != nullptr);
            attachment = std::make_unique<juce::SliderParameterAttachment> (*param, slider, nullptr);
            slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            slider.setTooltip (param->getName (64));

            slider.onValueChange = [this] { repaint(); };
            slider.onDragStart = [this] { dragging = true; repaint(); };
            slider.onDragEnd   = [this] { dragging = false; repaint(); };
            slider.addMouseListener (this, false);
        }

        void mouseEnter (const juce::MouseEvent&) override { hovering = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { hovering = false; repaint(); }

        void setLabel (const juce::String& text)
        {
            if (text != name) { name = text; repaint(); }
        }

        void setLabelHeight (int h) { labelHeight = h; resized(); }

        void resized() override
        {
            auto r = getLocalBounds();
            r.removeFromBottom (labelHeight);
            const int size = std::min (r.getWidth(), r.getHeight());
            slider.setBounds (r.withSizeKeepingCentre (size, size));
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            const bool showValue = hovering || dragging;
            g.setColour (showValue ? p.text : p.textDim);
            const auto text = showValue ? slider.getTextFromValue (slider.getValue()) : name.toUpperCase();
            drawSpacedText (g, text, getLocalBounds().removeFromBottom (labelHeight).toFloat(),
                            labelHeight > 18 ? 12.0f : 10.5f, juce::Justification::centred, showValue ? 0.02f : 0.12f);
        }

        int labelHeight = 16;

    private:
        juce::String name;
        KnobSlider slider;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
        bool hovering = false, dragging = false;
    };

    // Selector bar for choice parameters: text or small waveform glyphs.
    class Segmented : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        using GlyphFn = std::function<juce::Path (int index, juce::Rectangle<float> area)>;

        Segmented (APVTS& state, const juce::String& paramId, GlyphFn glyphs = nullptr, juce::StringArray labels = {})
            : glyph (std::move (glyphs))
        {
            auto* param = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId));
            jassert (param != nullptr);
            items = labels.isEmpty() ? param->choices : labels;
            attachment = std::make_unique<juce::ParameterAttachment> (
                *param, [this] (float v) { selected = juce::roundToInt (v); repaint(); }, nullptr);
            attachment->sendInitialUpdate();
            setTooltip (param->getName (64));
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            const float w = (float) getWidth() / (float) items.size();

            for (int i = 0; i < items.size(); ++i)
            {
                auto cell = juce::Rectangle<float> (w * (float) i, 0.0f, w, (float) getHeight()).reduced (2.0f, 1.0f);
                const bool on = i == selected;

                if (on)
                {
                    g.setColour (p.accentSoft);
                    g.fillRoundedRectangle (cell, 3.0f);
                }

                g.setColour (on ? p.accent : (i == hovered ? p.text : p.textDim));

                if (glyph != nullptr)
                    g.strokePath (glyph (i, cell.reduced (cell.getWidth() * 0.2f, cell.getHeight() * 0.28f)),
                                  juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                else
                    drawSpacedText (g, items[i].toUpperCase(), cell, 10.5f, juce::Justification::centred, 0.08f, on);
            }
        }

        void mouseMove (const juce::MouseEvent& e) override { setHovered (indexAt (e.x)); }
        void mouseExit (const juce::MouseEvent&) override   { setHovered (-1); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            attachment->setValueAsCompleteGesture ((float) indexAt (e.x));
        }

    private:
        int indexAt (int x) const
        {
            return std::clamp (x * items.size() / std::max (1, getWidth()), 0, items.size() - 1);
        }

        void setHovered (int i)
        {
            if (hovered != i) { hovered = i; repaint(); }
        }

        GlyphFn glyph;
        juce::StringArray items;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        int selected = 0, hovered = -1;
    };

    // Glyphs for oscillator waveforms: sine, triangle, saw, square, wavetable
    inline juce::Path waveGlyph (int index, juce::Rectangle<float> r)
    {
        juce::Path path;
        const float l = r.getX(), rt = r.getRight(), t = r.getY(), b = r.getBottom(), m = r.getCentreY();
        const float w = r.getWidth();

        auto sinePath = [&] (float x0, float y0, float width, float amp)
        {
            path.startNewSubPath (x0, y0);
            for (int i = 1; i <= 24; ++i)
            {
                const float x = (float) i / 24.0f;
                path.lineTo (x0 + x * width, y0 - std::sin (x * juce::MathConstants<float>::twoPi) * amp);
            }
        };

        switch (index)
        {
            case 0: sinePath (l, m, w, m - t); break;
            case 1:
                path.startNewSubPath (l, m);
                path.lineTo (l + w * 0.25f, t);
                path.lineTo (l + w * 0.75f, b);
                path.lineTo (rt, m);
                break;
            case 2:
                path.startNewSubPath (l, b);
                path.lineTo (l + w * 0.5f, t);
                path.lineTo (l + w * 0.5f, b);
                path.lineTo (rt, t);
                break;
            case 3:
                path.startNewSubPath (l, b);
                path.lineTo (l, t);
                path.lineTo (l + w * 0.5f, t);
                path.lineTo (l + w * 0.5f, b);
                path.lineTo (rt, b);
                path.lineTo (rt, t);
                break;
            default:   // Wavetable: staggered waves
            {
                const float h = r.getHeight();
                for (int k = 0; k < 3; ++k)
                    sinePath (l + (float) k * w * 0.12f, t + h * (0.3f + 0.25f * (float) k), w * 0.76f, h * 0.22f);
                break;
            }
        }
        return path;
    }

    // Glyphs for LFO shapes
    inline juce::Path lfoGlyph (int index, juce::Rectangle<float> r)
    {
        if (index < 2) return waveGlyph (index, r);
        juce::Path path;
        const float l = r.getX(), rt = r.getRight(), t = r.getY(), b = r.getBottom(), w = r.getWidth();
        switch (index)
        {
            case 2: path.startNewSubPath (l, b); path.lineTo (rt, t); path.lineTo (rt, b); break;   // Saw Up
            case 3: path.startNewSubPath (l, b); path.lineTo (l, t); path.lineTo (rt, b); break;    // Saw Down
            case 4: return waveGlyph (3, r);
            case 5:   // Sample & hold: steps
            {
                const float ys[] = { 0.6f, 0.15f, 0.85f, 0.35f };
                path.startNewSubPath (l, t + ys[0] * r.getHeight());
                for (int i = 0; i < 4; ++i)
                {
                    const float y = t + ys[i] * r.getHeight();
                    path.lineTo (l + w * (float) i / 4.0f, y);
                    path.lineTo (l + w * (float) (i + 1) / 4.0f, y);
                }
                break;
            }
            default:  // Smooth Random
            {
                path.startNewSubPath (l, r.getCentreY());
                path.cubicTo (l + w * 0.2f, t, l + w * 0.35f, t, l + w * 0.5f, r.getCentreY());
                path.cubicTo (l + w * 0.65f, b, l + w * 0.8f, b + 0.0f, rt, t + r.getHeight() * 0.3f);
                break;
            }
        }
        return path;
    }

    // Small round switch (on/off), e.g. for oscillators.
    class Toggle : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Toggle (APVTS& state, const juce::String& paramId, juce::String labelText = {})
            : label (std::move (labelText))
        {
            auto* param = state.getParameter (paramId);
            jassert (param != nullptr);
            attachment = std::make_unique<juce::ParameterAttachment> (
                *param, [this] (float v) { on = v > 0.5f; repaint(); if (onChange) onChange (on); }, nullptr);
            attachment->sendInitialUpdate();
            setTooltip (param->getName (64));
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat();
            const float d = std::min (12.0f, r.getHeight() - 2.0f);
            auto dot = r.removeFromLeft (d + 2.0f).withSizeKeepingCentre (d, d);

            g.setColour (on ? p.accent : p.line);
            if (on) g.fillEllipse (dot);
            else    g.drawEllipse (dot.reduced (0.5f), 1.2f);

            if (label.isNotEmpty())
            {
                g.setColour (on ? p.text : p.textDim);
                drawSpacedText (g, label.toUpperCase(), r.withTrimmedLeft (5.0f), 10.5f, juce::Justification::centredLeft, 0.1f);
            }
        }

        void mouseDown (const juce::MouseEvent&) override { attachment->setValueAsCompleteGesture (on ? 0.0f : 1.0f); }

        bool isOn() const { return on; }
        std::function<void (bool)> onChange;

    private:
        juce::String label;
        std::unique_ptr<juce::ParameterAttachment> attachment;
        bool on = false;
    };

    // Dropdown for longer choice lists.
    class Choice : public juce::Component
    {
    public:
        Choice (APVTS& state, const juce::String& paramId)
        {
            auto* param = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId));
            jassert (param != nullptr);
            box.addItemList (param->choices, 1);
            addAndMakeVisible (box);
            attachment = std::make_unique<juce::ComboBoxParameterAttachment> (*param, box);
        }

        void resized() override { box.setBounds (getLocalBounds()); }

    private:
        juce::ComboBox box;
        std::unique_ptr<juce::ComboBoxParameterAttachment> attachment;
    };

    // Calm panel with an all-caps title and a fine outline.
    class Section : public juce::Component
    {
    public:
        explicit Section (juce::String t) : title (std::move (t)) {}

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (p.panel);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (p.line);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);

            g.setColour (p.text);
            drawSpacedText (g, title.toUpperCase(), r.reduced (16.0f, 0.0f).removeFromTop (34.0f),
                            11.0f, juce::Justification::centredLeft, 0.25f, true);
        }

        // Content area below the title
        juce::Rectangle<int> content() const { return getLocalBounds().reduced (12, 0).withTrimmedTop (34).withTrimmedBottom (10); }

        juce::Rectangle<int> titleArea() const { return getLocalBounds().reduced (16, 0).removeFromTop (34); }

        // A small switch without label at the right end of the title (e.g. a lock)
        void placeTitleLock (juce::Component& lock) const { lock.setBounds (titleArea().removeFromRight (18).withSizeKeepingCentre (18, 18)); }

    private:
        juce::String title;
    };

    // Padlock (closed / open), used for the lock switches.
    inline void drawPadlock (juce::Graphics& g, juce::Rectangle<float> area, bool closed, juce::Colour c)
    {
        const auto r = area.withSizeKeepingCentre (10.0f, 12.0f);
        const auto body = r.withTrimmedTop (5.0f);
        juce::Path shackle;
        const float sx = body.getX() + 2.0f, sw = body.getWidth() - 4.0f, top = r.getY();
        shackle.startNewSubPath (sx, body.getY());
        shackle.lineTo (sx, top + sw * 0.5f);
        shackle.addCentredArc (sx + sw * 0.5f, top + sw * 0.5f, sw * 0.5f, sw * 0.5f, 0.0f,
                               -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi);
        shackle.lineTo (sx + sw, closed ? body.getY() : top + sw * 0.5f + 1.5f);
        g.setColour (c);
        g.strokePath (shackle, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (closed) g.fillRoundedRectangle (body, 1.5f);
        else        g.drawRoundedRectangle (body.reduced (0.5f), 1.5f, 1.3f);
    }

    // Padlock switch for one lock group (state property): a locked part of the sound stays when another sound preset is
    // loaded (see presets/Locks.h). Typically sits in a panel's title (Section::placeTitleLock).
    class LockButton : public juce::Component, public juce::SettableTooltipClient, private juce::Value::Listener
    {
    public:
        LockButton (APVTS& state, const juce::String& property, const juce::String& partName, bool showLabel = true)
            : withLabel (showLabel)
        {
            value.referTo (state.state.getPropertyAsValue (property, nullptr));
            value.addListener (this);
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTooltip ("Lock: keep the " + partName + " settings when switching sound presets");
        }

        LockButton (APVTS& state, const locks::LockSet& set, int group, bool showLabel = true)
            : LockButton (state, set[group].property, set[group].label, showLabel) {}

        ~LockButton() override { value.removeListener (this); }

        bool isLocked() const { return (bool) value.getValue(); }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            const bool locked = isLocked();
            auto r = getLocalBounds().toFloat();
            const auto colour = locked ? p.accent : (hover ? p.text : p.textDim);
            drawPadlock (g, r.removeFromLeft (16.0f), locked, colour);
            if (withLabel)
            {
                g.setColour (locked || hover ? p.text : p.textDim);
                drawSpacedText (g, locked ? "LOCKED" : "LOCK", r.withTrimmedLeft (5.0f), 10.0f,
                                juce::Justification::centredLeft, 0.14f, locked);
            }
        }

        void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
        void mouseDown (const juce::MouseEvent&) override  { value = ! isLocked(); }

    private:
        void valueChanged (juce::Value&) override { repaint(); }

        juce::Value value;
        bool withLabel, hover = false;
    };

    // Lays out components evenly in a row.
    inline void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps)
    {
        const int n = (int) comps.size();
        const int w = area.getWidth() / std::max (1, n);
        for (auto* c : comps)
            if (c != nullptr)
                c->setBounds (area.removeFromLeft (w));
            else
                area.removeFromLeft (w);
    }

    // A panel with a title, used by most pages.
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, float radius = 8.0f)
    {
        const auto& p = Theme::get().palette();
        g.setColour (p.panel);
        g.fillRoundedRectangle (r, radius);
        g.setColour (p.line);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        if (title.isNotEmpty())
        {
            g.setColour (p.text);
            drawSpacedText (g, title.toUpperCase(), r.reduced (16.0f, 0.0f).removeFromTop (32.0f), 10.5f,
                            juce::Justification::centredLeft, 0.25f, true);
        }
    }

    // Small label above a selector (segmented / dropdown / toggle)
    class Labelled : public juce::Component
    {
    public:
        Labelled (std::unique_ptr<juce::Component> c, juce::String t) : content (std::move (c)), title (std::move (t))
        {
            addAndMakeVisible (*content);
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (Theme::get().palette().textDim);
            drawSpacedText (g, title.toUpperCase(), getLocalBounds().removeFromTop (14).toFloat(), 9.5f,
                            juce::Justification::centredLeft, 0.16f);
        }

        void resized() override { content->setBounds (getLocalBounds().withTrimmedTop (16).withHeight (24)); }

        juce::Component& get() { return *content; }

    private:
        std::unique_ptr<juce::Component> content;
        juce::String title;
    };

    inline std::unique_ptr<juce::Component> labelled (std::unique_ptr<juce::Component> c, const juce::String& title)
    {
        return std::make_unique<Labelled> (std::move (c), title);
    }

    // A row of controls: each entry has a relative width (knobs 1, wide selectors 2-4).
    struct ControlRow
    {
        std::vector<std::pair<std::unique_ptr<juce::Component>, float>> items;

        void add (std::unique_ptr<juce::Component> c, float units = 1.0f) { items.emplace_back (std::move (c), units); }

        void addTo (juce::Component& parent)
        {
            for (auto& [c, u] : items) parent.addAndMakeVisible (*c);
        }

        void setVisible (bool v) { for (auto& [c, u] : items) c->setVisible (v); }

        // Selectors get a fixed height and sit centred in their cell
        void layout (juce::Rectangle<int> area, int gap = 4)
        {
            float total = 0.0f;
            for (auto& [c, u] : items) total += u;
            const float unit = (float) (area.getWidth() - gap * ((int) items.size() - 1)) / std::max (1.0f, total);
            float x = (float) area.getX();
            for (auto& [c, u] : items)
            {
                const int w = (int) (unit * u);
                juce::Rectangle<int> cell ((int) x, area.getY(), w, area.getHeight());
                if (dynamic_cast<Knob*> (c.get()) == nullptr)
                {
                    const int h = dynamic_cast<Labelled*> (c.get()) != nullptr ? 40 : 24;
                    cell = cell.withSizeKeepingCentre (w, h).translated (0, -6);
                }
                c->setBounds (cell);
                x += (float) w + (float) gap;
            }
        }

        // Rows of at most 'perRow' units: knob rows are taller than selector rows
        void layoutGrid (juce::Rectangle<int> area, int perRow = 4, int knobRowHeight = 84, int selectorRowHeight = 48)
        {
            const int unitW = area.getWidth() / perRow;
            int used = 0;
            std::vector<std::pair<juce::Component*, int>> line;
            auto place = [&]
            {
                if (line.empty()) return;
                bool anyKnob = false;
                for (auto& [c, u] : line) anyKnob |= dynamic_cast<Knob*> (c) != nullptr;
                auto rowArea = area.removeFromTop (anyKnob ? knobRowHeight : selectorRowHeight);
                for (auto& [c, u] : line)
                {
                    auto cell = rowArea.removeFromLeft (unitW * u);
                    if (dynamic_cast<Knob*> (c) == nullptr) cell = cell.withSizeKeepingCentre (cell.getWidth() - 8, 40);
                    c->setBounds (cell);
                }
                area.removeFromTop (4);
                line.clear();
                used = 0;
            };
            for (auto& [c, units] : items)
            {
                const int u = juce::jlimit (1, perRow, (int) std::ceil (units - 0.25f));
                if (used + u > perRow) place();
                line.emplace_back (c.get(), u);
                used += u;
            }
            place();
        }
    };

    // Simple pill button (primary = filled with the accent).
    class PillButton : public juce::Component
    {
    public:
        PillButton (juce::String t, bool isPrimary) : text (std::move (t)), primary (isPrimary)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        void setText (const juce::String& t) { text = t; repaint(); }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            const float rad = r.getHeight() * 0.5f;
            if (primary)
            {
                g.setColour (hover ? p.accent.brighter (0.1f) : p.accent);
                g.fillRoundedRectangle (r, rad);
                g.setColour (p.panel);
            }
            else
            {
                g.setColour (hover ? p.textDim : p.line);
                g.drawRoundedRectangle (r, rad, 1.0f);
                g.setColour (p.text);
            }
            drawSpacedText (g, text.toUpperCase(), r, 10.5f, juce::Justification::centred, 0.18f, true);
        }

        void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
        }

        std::function<void()> onClick;

    private:
        juce::String text;
        bool primary, hover = false;
    };

    inline void styleTextEditor (juce::TextEditor& ed)
    {
        const auto& p = Theme::get().palette();
        ed.setColour (juce::TextEditor::backgroundColourId, p.background);
        ed.setColour (juce::TextEditor::textColourId, p.text);
        ed.setColour (juce::TextEditor::outlineColourId, p.line);
        ed.setColour (juce::TextEditor::focusedOutlineColourId, p.accent);
        ed.setColour (juce::TextEditor::highlightColourId, p.accentSoft);
        ed.setColour (juce::TextEditor::highlightedTextColourId, p.text);
        ed.setColour (juce::CaretComponent::caretColourId, p.accent);
        ed.setTextToShowWhenEmpty (ed.getTextToShowWhenEmpty(), p.textDim);
        ed.setFont (font (14.0f));
        ed.setIndents (18, 0);
        ed.setJustification (juce::Justification::centredLeft);
        ed.applyFontToAllText (font (14.0f));
        ed.applyColourToAllText (p.text);
    }

    inline juce::Path starPath (juce::Rectangle<float> r)
    {
        juce::Path p;
        p.addStar (r.getCentre(), 5, r.getWidth() * 0.22f, r.getWidth() * 0.5f, 0.0f);
        return p;
    }

    inline void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour c, float width, bool glow)
    {
        if (glow)
        {
            g.setColour (c.withAlpha (0.12f));
            g.strokePath (path, juce::PathStrokeType (width * 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Log frequency axis 20 Hz .. 20 kHz
    inline float freqToX (float hz, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log (hz / 20.0f) / std::log (1000.0f); }
    inline float xToFreq (float x, juce::Rectangle<float> r) { return 20.0f * std::pow (1000.0f, (x - r.getX()) / r.getWidth()); }
}
