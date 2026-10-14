#pragma once
#include "PresetBar.h"
#include "../presets/Locks.h"

namespace fjord::base::ui
{
    // The menu behind the padlock in the preset bar: parts of the sound that stay when another preset is loaded. The plugin
    // lists its groups in a locks::LockSet and describes the menu once:
    //
    //   MenuLayout layout;
    //   layout.header ("Keep when switching presets")
    //         .submenu ("Layers", { body, click, sub, noise }, "All layers")
    //         .item (kick)
    //         .separator()
    //         .effects (locks)                      // the shared effect locks (whole chain + one per effect)
    //         .all ("All step data", { pattern });
    //   LockMenu::attach (presetBar, apvts, locks, layout);   // padlock glyph, tooltip, repaint, menu, "Options …"
    class MenuLayout
    {
    public:
        struct Entry
        {
            enum class Type { Header, Item, Separator, Submenu, All } type = Type::Item;
            juce::String label, allLabel;
            int group = -1;
            std::vector<int> groups;   // Submenu / All: the groups (-1 = separator line inside a submenu)
        };

        MenuLayout& header (juce::String text)  { entries.push_back ({ Entry::Type::Header, std::move (text), {}, -1, {} }); return *this; }
        MenuLayout& item (int group)            { entries.push_back ({ Entry::Type::Item, {}, {}, group, {} }); return *this; }
        MenuLayout& separator()                 { entries.push_back ({ Entry::Type::Separator, {}, {}, -1, {} }); return *this; }
        // A submenu with a check for every group; 'allLabel' adds an entry below that locks them all.
        MenuLayout& submenu (juce::String name, std::vector<int> groups, juce::String allLabel = {})
        {
            entries.push_back ({ Entry::Type::Submenu, std::move (name), std::move (allLabel), -1, std::move (groups) });
            return *this;
        }
        // An entry that locks several groups at once.
        MenuLayout& all (juce::String label, std::vector<int> groups)
        {
            entries.push_back ({ Entry::Type::All, std::move (label), {}, -1, std::move (groups) });
            return *this;
        }
        // The "Effects" submenu: the whole chain (incl. order), a separator and one entry per effect.
        MenuLayout& effects (const locks::LockSet& set, const juce::String& name = "Effects")
        {
            std::vector<int> groups { set.effectGroup (-1), -1 };
            for (int m = 0; m < fx::kNumModules; ++m) groups.push_back (set.effectGroup (m));
            return submenu (name, std::move (groups));
        }

        std::vector<Entry> entries;
    };

    struct LockMenu
    {
        static constexpr int unlockId = 900, optionsId = 901, actionBase = 1000;

        static juce::String tooltip (const juce::ValueTree& state, const locks::LockSet& set)
        {
            const auto locked = set.lockedLabels (state);
            return locked.isEmpty() ? juce::String ("Lock parts of the sound while browsing presets")
                                    : "Kept when switching presets: " + locked.joinIntoString (", ");
        }

        // Sets the preset bar up: padlock glyph (closed while something is locked), tooltip, repaint on changes, the menu.
        static void attach (PresetBar& bar, juce::AudioProcessorValueTreeState& apvts, const locks::LockSet& set, MenuLayout layout)
        {
            auto shared = std::make_shared<MenuLayout> (std::move (layout));
            auto state = apvts.state;
            bar.menuStyle = PresetBar::MenuStyle::Padlock;
            bar.isMenuActive = [state, &set] { return ! set.lockedLabels (state).isEmpty(); };
            bar.menuTooltip = [state, &set] { return tooltip (state, set); };
            bar.onMenu = [state, &set, shared, &bar] (juce::Component&) { show (state, set, *shared, bar); };
            for (int g = 0; g < set.size(); ++g)
                bar.watch (state.getPropertyAsValue (set[g].property, nullptr));
        }

        static void show (juce::ValueTree state, const locks::LockSet& set, const MenuLayout& layout, PresetBar& bar)
        {
            using Type = MenuLayout::Entry::Type;
            std::vector<std::vector<int>> actions;   // "lock all" entries: id = actionBase + index

            auto addGroup = [&] (juce::PopupMenu& m, int g)
            {
                if (g < 0) m.addSeparator();
                else       m.addItem (g + 1, set[g].label, true, set.isLocked (state, g));
            };
            auto addAction = [&] (juce::PopupMenu& m, const juce::String& label, std::vector<int> groups)
            {
                actions.push_back (std::move (groups));
                m.addItem (actionBase + (int) actions.size() - 1, label);
            };

            juce::PopupMenu menu;
            for (const auto& e : layout.entries)
            {
                switch (e.type)
                {
                    case Type::Header:    menu.addSectionHeader (e.label); break;
                    case Type::Separator: menu.addSeparator(); break;
                    case Type::Item:      addGroup (menu, e.group); break;
                    case Type::All:       addAction (menu, e.label, e.groups); break;
                    case Type::Submenu:
                    {
                        juce::PopupMenu sub;
                        bool any = false;
                        for (int g : e.groups)
                        {
                            addGroup (sub, g);
                            any = any || (g >= 0 && set.isLocked (state, g));
                        }
                        if (e.allLabel.isNotEmpty())
                        {
                            sub.addSeparator();
                            std::vector<int> real;
                            for (int g : e.groups) if (g >= 0) real.push_back (g);
                            addAction (sub, e.allLabel, std::move (real));
                        }
                        menu.addSubMenu (e.label, sub, true, nullptr, any);
                        break;
                    }
                }
            }
            menu.addSeparator();
            menu.addItem (unlockId, "Unlock all");
            menu.addSeparator();
            menu.addItem (optionsId, juce::String (juce::CharPointer_UTF8 ("Options \xe2\x80\xa6")));

            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&bar),
                                [state, &set, actions, safe = juce::Component::SafePointer<PresetBar> (&bar)] (int r) mutable
            {
                if (r >= 1 && r <= set.size())
                    set.setLocked (state, r - 1, ! set.isLocked (state, r - 1));
                else if (r >= actionBase && r < actionBase + (int) actions.size())
                    for (int g : actions[(size_t) (r - actionBase)]) set.setLocked (state, g, true);
                else if (r == unlockId)
                    set.unlockAll (state);
                if (safe != nullptr)
                {
                    if (r == optionsId) safe->showOptions();
                    safe->repaint();
                }
            });
        }
    };
}
