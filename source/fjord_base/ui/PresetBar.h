#pragma once
#include "Theme.h"
#include "Controls.h"
#include "Icons.h"
#include "../presets/PresetManagerBase.h"

namespace fjord::base::ui
{
    using PresetManagerBase = fjord::base::presets::PresetManagerBase;

    // Horizontal preset bar:  ‹   TECHNO · Granite   ›  |  ···  ⤓
    // Clicking the centre opens the preset browser, the menu zone opens the options (or the plugin's own menu, e.g.
    // Fjord's lock menu with a padlock glyph), the icon on the right saves.
    class PresetBar : public juce::Component,
                      public juce::SettableTooltipClient,
                      private juce::ChangeListener,
                      private juce::Value::Listener
    {
    public:
        enum class MenuStyle { Dots, Padlock };

        explicit PresetBar (PresetManagerBase& pm) : presets (pm)
        {
            presets.addChangeListener (this);
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        ~PresetBar() override
        {
            presets.removeChangeListener (this);
            for (auto& v : watched) v.removeListener (this);
        }

        std::function<void()> onOpenBrowser, onSave;

        // Plugin hooks for the menu zone: a whole custom menu, or extra entries for the standard options menu
        // (use ids >= 100 for them).
        MenuStyle menuStyle = MenuStyle::Dots;
        std::function<void (juce::Component& target)> onMenu;
        std::function<bool()> isMenuActive;               // padlock drawn closed and in the accent colour
        std::function<juce::String()> menuTooltip;
        std::function<void (juce::PopupMenu&)> addOptions;
        std::function<void (int)> handleOption;

        // Repaint when one of these values changes (e.g. lock properties in the state)
        void watch (const juce::Value& v)
        {
            watched.emplace_back().referTo (v);
            watched.back().addListener (this);
        }

        void setBrowserOpen (bool open) { browserOpen = open; repaint(); }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            const float rad = r.getHeight() * 0.5f;

            g.setColour (p.panel);
            g.fillRoundedRectangle (r, rad);
            g.setColour (browserOpen ? p.accent : (hover != Zone::None ? p.textDim : p.line));
            g.drawRoundedRectangle (r, rad, 1.0f);

            icons::drawChevron (g, zoneBounds (Zone::Prev).toFloat(), true,  hover == Zone::Prev ? p.accent : p.textDim);
            icons::drawChevron (g, zoneBounds (Zone::Next).toFloat(), false, hover == Zone::Next ? p.accent : p.textDim);

            // Divider, menu + save icons
            const auto menu = zoneBounds (Zone::Menu).toFloat();
            const auto save = zoneBounds (Zone::Save).toFloat();
            g.setColour (p.line);
            g.fillRect (menu.getX(), menu.getY() + 9.0f, 1.0f, menu.getHeight() - 18.0f);
            if (menuStyle == MenuStyle::Padlock)
            {
                const bool active = isMenuActive != nullptr && isMenuActive();
                drawPadlock (g, menu.withTrimmedLeft (4.0f), active, active ? p.accent : (hover == Zone::Menu ? p.text : p.textDim));
            }
            else
            {
                g.setColour (hover == Zone::Menu ? p.accent : p.textDim);
                for (int i = -1; i <= 1; ++i)
                    g.fillEllipse (menu.getCentreX() + 2.0f + (float) i * 5.0f - 1.3f, menu.getCentreY() - 1.3f, 2.6f, 2.6f);
            }
            icons::drawSave (g, save, hover == Zone::Save ? p.accent : p.textDim);

            // Category + name, centred together
            const auto category = categoryLabel (presets.getCurrentCategory()).toUpperCase();
            const auto name = presets.getCurrentName();
            const auto catFont = font (10.5f, true).withExtraKerningFactor (0.22f);
            const auto nameFont = font (15.0f);
            const float catW = category.isEmpty() ? 0.0f : juce::GlyphArrangement::getStringWidth (catFont, category) + 22.0f;
            const float nameW = juce::GlyphArrangement::getStringWidth (nameFont, name);
            auto centre = zoneBounds (Zone::Centre).toFloat();
            float x = std::max (centre.getX(), centre.getCentreX() - (catW + nameW) * 0.5f);

            if (category.isNotEmpty())
            {
                g.setColour (p.accent);
                g.setFont (catFont);
                g.drawText (category, juce::Rectangle<float> (x, centre.getY(), catW, centre.getHeight()),
                            juce::Justification::centredLeft, false);
                g.setColour (p.line);
                g.fillEllipse (x + catW - 13.0f, centre.getCentreY() - 1.5f, 3.0f, 3.0f);
                x += catW;
            }

            g.setColour (p.text);
            g.setFont (nameFont);
            g.drawText (name, juce::Rectangle<float> (x, centre.getY(), centre.getRight() - x, centre.getHeight()),
                        juce::Justification::centredLeft, true);
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const auto z = zoneAt (e.getPosition());
            if (z != hover)
            {
                hover = z;
                setTooltip (z == Zone::Save ? "Save preset" : z == Zone::Centre ? "Browse presets"
                          : z == Zone::Menu ? (menuTooltip != nullptr ? menuTooltip() : juce::String ("Options")) : juce::String());
                repaint();
            }
        }

        void mouseExit (const juce::MouseEvent&) override { hover = Zone::None; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            switch (zoneAt (e.getPosition()))
            {
                case Zone::Prev:   presets.previous(); break;
                case Zone::Next:   presets.next();     break;
                case Zone::Centre: if (onOpenBrowser) onOpenBrowser(); break;
                case Zone::Save:   if (onSave) onSave(); break;
                case Zone::Menu:   if (onMenu) onMenu (*this); else showOptions(); break;
                case Zone::None:   break;
            }
        }

        // The standard options menu (also reachable from plugins that use their own menu zone)
        void showOptions()
        {
            juce::PopupMenu m;
            m.addItem (1, "Init sound");
            if (presets.getConfig().hasPatterns)
            {
                m.addSeparator();
                m.addItem (2, presets.getConfig().patternLabel, true, presets.getLoadPatterns());
            }
            if (addOptions != nullptr)
            {
                m.addSeparator();
                addOptions (m);
            }
            m.addSeparator();
            m.addItem (3, "Show user preset folder");
            m.addItem (4, "Rescan presets");
            m.addSeparator();
            // Window size: the same scaling as dragging the window corner (75 - 150 %), for hosts without a resize handle
            juce::PopupMenu sizes;
            const int currentPercent = juce::roundToInt (Theme::get().getScale() * 100.0f);
            static constexpr int percents[] = { 75, 100, 125, 150 };
            for (int i = 0; i < 4; ++i)
                sizes.addItem (10 + i, juce::String (percents[i]) + " %", true, std::abs (currentPercent - percents[i]) < 3);
            m.addSubMenu ("Window size", sizes);
            m.addItem (5, "Reduce motion", true, Theme::get().reduceMotion());
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
            {
                static constexpr int percents[] = { 75, 100, 125, 150 };
                if (r == 1) presets.loadInit();
                if (r == 2) presets.setLoadPatterns (! presets.getLoadPatterns());
                if (r == 3)
                {
                    auto folder = PresetManagerBase::getUserPresetFolder();
                    folder.createDirectory();
                    folder.revealToUser();
                }
                if (r == 4) presets.rescan();
                if (r == 5) Theme::get().setFlag ("reduceMotion", ! Theme::get().reduceMotion());
                if (r >= 10 && r < 14) setWindowScale ((float) percents[r - 10] / 100.0f);
                if (r >= 100 && handleOption != nullptr) handleOption (r);
                repaint();
            });
        }

    private:
        // Resizes the plugin window to a scale (the editor keeps its aspect ratio; ScaledEditor stores the size)
        void setWindowScale (float target)
        {
            if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
            {
                const float current = std::max (0.1f, Theme::get().getScale());
                editor->setSize (juce::roundToInt ((float) editor->getWidth() * target / current),
                                 juce::roundToInt ((float) editor->getHeight() * target / current));
            }
        }

        enum class Zone { None, Prev, Centre, Next, Menu, Save };

        juce::Rectangle<int> zoneBounds (Zone z) const
        {
            auto r = getLocalBounds();
            const int h = getHeight();
            auto save = r.removeFromRight (h);
            auto menu = r.removeFromRight (h - 2);
            auto prev = r.removeFromLeft (h + 4);
            auto next = r.removeFromRight (h);
            switch (z)
            {
                case Zone::Menu: return menu;
                case Zone::Prev: return prev;
                case Zone::Next: return next;
                case Zone::Save: return save;
                case Zone::Centre: return r;
                case Zone::None: break;
            }
            return {};
        }

        Zone zoneAt (juce::Point<int> pos) const
        {
            for (auto z : { Zone::Prev, Zone::Next, Zone::Menu, Zone::Save, Zone::Centre })
                if (zoneBounds (z).contains (pos)) return z;
            return Zone::None;
        }

        void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
        void valueChanged (juce::Value&) override { repaint(); }

        PresetManagerBase& presets;
        std::vector<juce::Value> watched;
        Zone hover = Zone::None;
        bool browserOpen = false;
    };
}
