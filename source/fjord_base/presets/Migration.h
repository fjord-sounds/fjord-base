#pragma once
#include <juce_data_structures/juce_data_structures.h>

// Converting old parameter IDs and values in presets and saved projects.
//
// A plugin keeps one table per change (old ID -> new ID, the old default and an optional value conversion). Old
// presets (<Param id value/> children) and old project states (APVTS <PARAM id value/> children) are rewritten before
// they are applied: renamed values keep their meaning, and parameters the old file did not contain get the OLD default
// (so a sound that relied on an old default still sounds the same with the new parameter).
namespace fjord::base::migration
{
    struct Rename
    {
        juce::String from, to;
        float oldDefault = 0.0f;
        std::function<float (float)> convert = {};
    };

    // Values written when any old parameter is present (new parameters whose neutral setting differs from the default)
    struct Fixed
    {
        juce::String id;
        float value = 0.0f;
    };

    struct Table
    {
        std::vector<Rename> renames;
        std::vector<Fixed> fixed;
    };

    namespace detail
    {
        template <typename GetFn, typename SetFn, typename RemoveFn, typename HasFn>
        bool apply (const Table& t, GetFn get, SetFn set, RemoveFn remove, HasFn has)
        {
            bool legacy = false;
            for (const auto& r : t.renames)
                legacy = legacy || has (r.from);
            if (! legacy)
                return false;

            for (const auto& r : t.renames)
            {
                const float old = has (r.from) ? get (r.from) : r.oldDefault;
                remove (r.from);
                if (! has (r.to))
                    set (r.to, r.convert != nullptr ? r.convert (old) : old);
            }
            for (const auto& f : t.fixed)
                if (! has (f.id))
                    set (f.id, f.value);
            return true;
        }
    }

    // Preset XML: <Param id="..." value="..."/>. Returns true if anything was converted.
    inline bool migratePreset (juce::XmlElement& xml, const Table& t)
    {
        auto find = [&xml] (const juce::String& id) -> juce::XmlElement*
        {
            for (auto* e : xml.getChildWithTagNameIterator ("Param"))
                if (e->getStringAttribute ("id") == id) return e;
            return nullptr;
        };
        return detail::apply (t,
            [&] (const juce::String& id) { return (float) find (id)->getDoubleAttribute ("value"); },
            [&] (const juce::String& id, float v) { auto* e = xml.createNewChildElement ("Param"); e->setAttribute ("id", id); e->setAttribute ("value", v); },
            [&] (const juce::String& id) { if (auto* e = find (id)) xml.removeChildElement (e, true); },
            [&] (const juce::String& id) { return find (id) != nullptr; });
    }

    // Saved project state (AudioProcessorValueTreeState): <PARAM id="..." value="..."/> children of the root.
    inline bool migrateState (juce::ValueTree& state, const Table& t)
    {
        const juce::Identifier param ("PARAM"), idProp ("id"), valueProp ("value");
        auto find = [&] (const juce::String& id) { return state.getChildWithProperty (idProp, id); };
        return detail::apply (t,
            [&] (const juce::String& id) { return (float) find (id).getProperty (valueProp); },
            [&] (const juce::String& id, float v) { juce::ValueTree c (param); c.setProperty (idProp, id, nullptr); c.setProperty (valueProp, v, nullptr); state.appendChild (c, nullptr); },
            [&] (const juce::String& id) { if (auto c = find (id); c.isValid()) state.removeChild (c, nullptr); },
            [&] (const juce::String& id) { return find (id).isValid(); });
    }

    // Choice index conversion from a list: newIndex = map[oldIndex]
    inline std::function<float (float)> remapChoice (std::vector<int> map)
    {
        return [map] (float v)
        {
            const int i = juce::jlimit (0, (int) map.size() - 1, (int) std::lround (v));
            return (float) map[(size_t) i];
        };
    }

    inline std::function<float (float)> scale (float factor) { return [factor] (float v) { return v * factor; }; }
}
