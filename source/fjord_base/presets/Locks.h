#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include "../fx/FxParams.h"

namespace fjord::base::locks
{
    // Parts of a sound that can be locked: a locked part keeps its values when another sound preset is loaded, e.g. an
    // arp pattern, one oscillator, the envelopes or an effect chain you want to try on other sounds. The switches are
    // properties of the plugin state (saved with the project). Every plugin lists its own groups once (LockSet) and
    // describes the padlock menu (MenuLayout in ui/LockMenu.h); the effects come with the base (addEffectGroups).
    //
    // A group protects the parameters whose ids start with one of its prefixes (separated by '|') and, for plugin data
    // that is not a parameter (envelopes, patterns, key edits …), whatever the plugin's PresetManager protects when it
    // asks isLockedProperty() in applyExtras.
    struct Group
    {
        juce::String property;   // state property, e.g. "osc1Lock" (stays the same forever: it is saved in projects)
        juce::String label;      // menu text
        juce::String prefixes;   // parameter id prefixes, e.g. "osc1_" or "sub_|noise_" (empty: data without parameters)
    };

    class LockSet
    {
    public:
        // Returns the index of the group (used by LockButton and the menu layout).
        int add (Group g)
        {
            groups.push_back ({ std::move (g.property), std::move (g.label), std::move (g.prefixes) });
            return (int) groups.size() - 1;
        }

        // "Whole effect chain" (property "fxLock", also behind the Lock FX switch of the FX page, keeps the order too) and one
        // lock per effect (fxLock_<effect id>). Returns the index of the whole-chain group; the effects follow in
        // fx::Module order (effectGroup).
        int addEffectGroups()
        {
            firstEffectGroup = add ({ "fxLock", "Whole effect chain (incl. order)", "fx_" });
            for (int m = 0; m < fx::kNumModules; ++m)
            {
                const juce::String id = fx::info (m).id;
                add ({ "fxLock_" + id, fx::info (m).name, "fx_" + id + "_" });
            }
            return firstEffectGroup;
        }

        int effectGroup (int module) const { return firstEffectGroup + 1 + module; }
        int size() const { return (int) groups.size(); }
        const Group& operator[] (int i) const { return groups[(size_t) i]; }
        int find (const juce::String& property) const
        {
            for (int g = 0; g < size(); ++g)
                if (groups[(size_t) g].property == property) return g;
            return -1;
        }

        bool isLocked (const juce::ValueTree& state, int group) const
        {
            return (bool) state.getProperty (groups[(size_t) group].property, false);
        }

        void setLocked (juce::ValueTree state, int group, bool locked) const
        {
            state.setProperty (groups[(size_t) group].property, locked, nullptr);
        }

        // For plugin data without parameters: is the group with this property locked?
        static bool isLockedProperty (const juce::ValueTree& state, const juce::String& property)
        {
            return (bool) state.getProperty (property, false);
        }

        bool matches (int group, const juce::String& paramId) const
        {
            for (const auto& prefix : juce::StringArray::fromTokens (groups[(size_t) group].prefixes, "|", ""))
                if (prefix.isNotEmpty() && paramId.startsWith (prefix)) return true;
            return false;
        }

        // Is this parameter protected by an active lock?
        bool isParameterLocked (const juce::ValueTree& state, const juce::String& paramId) const
        {
            for (int g = 0; g < size(); ++g)
                if (isLocked (state, g) && matches (g, paramId)) return true;
            return false;
        }

        juce::StringArray lockedLabels (const juce::ValueTree& state) const
        {
            juce::StringArray a;
            for (int g = 0; g < size(); ++g)
                if (isLocked (state, g)) a.add (groups[(size_t) g].label);
            return a;
        }

        void unlockAll (juce::ValueTree state) const
        {
            for (int g = 0; g < size(); ++g) setLocked (state, g, false);
        }

    private:
        std::vector<Group> groups;
        int firstEffectGroup = 0;
    };
}
