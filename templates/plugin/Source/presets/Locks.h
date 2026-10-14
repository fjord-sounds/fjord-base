#pragma once
#include <fjord_base/presets/Locks.h>
#include <fjord_base/ui/LockMenu.h>

namespace @name@::locks
{
    // Parts of a sound that stay when another sound preset is loaded: an oscillator, a filter, an envelope, the effect
    // chain … The mechanism (LockSet, padlock menu, LockButton, preset manager hook) is fjord-base; this is the plugin's
    // list. The switches are properties of the project state – their names never change. A group protects the
    // parameters whose ids start with one of its prefixes; plugin data that is not a parameter (envelopes, patterns …) is
    // protected in PresetManager::applyExtras via isLockedProperty().
    struct Ids
    {
        int osc, filter, amp;
    };

    inline const std::pair<const fjord::base::locks::LockSet&, Ids>& setup()
    {
        static const auto pair = []
        {
            static fjord::base::locks::LockSet set;
            Ids ids {};
            ids.osc    = set.add ({ "oscLock",    "Oscillator", "osc_" });
            ids.filter = set.add ({ "filterLock", "Filter",     "filter_" });
            ids.amp    = set.add ({ "ampLock",    "Amp envelope", "amp_" });
            set.addEffectGroups();   // the whole chain and one lock per effect – the same in every plugin
            return std::pair<const fjord::base::locks::LockSet&, Ids> { set, ids };
        }();
        return pair;
    }

    inline const fjord::base::locks::LockSet& set() { return setup().first; }
    inline const Ids& ids() { return setup().second; }

    // The padlock menu: what the plugin offers to keep (submenus, "all" entries and separators are described here)
    inline fjord::base::ui::MenuLayout menuLayout()
    {
        const auto& i = ids();
        fjord::base::ui::MenuLayout layout;
        layout.header ("Keep when switching presets")
              .item (i.osc)
              .item (i.filter)
              .item (i.amp)
              .separator()
              .effects (set());
        return layout;
    }
}
