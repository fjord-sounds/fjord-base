#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FjordBaseData.h"
#include "FxParams.h"

namespace fjord::base::fx
{
    // Presets for a single effect (.fjordmod): only the parameters of that one effect, by their short key:
    //   <FjordFxPresets effect="reverb"><Preset name="Small Room" category="Rooms"><Param id="size" value="0.25"/>…
    // Factory presets live in fjord-base/presets/fx/<effect>.fjordmod (embedded in every plugin), user presets in
    // ~/Music/Fjord/FX Presets/<effect>/<name>.fjordmod – shared by all plugins and the single-effect plugins.
    // Parameters a preset leaves out get their default.
    class ModulePresets
    {
    public:
        struct Preset
        {
            juce::String name, category;
            std::map<juce::String, float> values;   // short keys ("size", "mix" …)
            juce::File file;                         // empty for factory presets
        };

        static ModulePresets& get()
        {
            static ModulePresets instance;
            return instance;
        }

        // Screenshots / tests: only the factory presets
        static inline bool ignoreUserPresets = false;

        const std::vector<Preset>& getPresets (int module)
        {
            const auto m = (size_t) juce::jlimit (0, kNumModules - 1, module);
            if (! loaded[m]) load ((int) m);
            return presets[m];
        }

        static juce::File getUserFolder (int module)
        {
            return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                .getChildFile ("Fjord/FX Presets").getChildFile (info (module).id);
        }

        static juce::String prefix (int module) { return "fx_" + juce::String (info (module).id) + "_"; }

        // Applies a preset to one effect and switches the effect on.
        static void apply (juce::AudioProcessorValueTreeState& state, int module, const Preset& preset, juce::UndoManager* undo = nullptr)
        {
            if (undo != nullptr) undo->beginNewTransaction (juce::String (info (module).name) + " preset");
            const auto pre = prefix (module);
            for (int p = info (module).first; p <= info (module).last; ++p)
                if (auto* param = state.getParameter (paramId ((Param) p)))
                {
                    const auto key = param->getParameterID().substring (pre.length());
                    const auto it = preset.values.find (key);
                    const float value = key == "on" ? 1.0f
                                      : (it != preset.values.end() ? param->convertTo0to1 (it->second) : param->getDefaultValue());
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (value);
                    param->endChangeGesture();
                }
        }

        // Do the current settings of the effect equal this preset (the on switch is ignored)?
        static bool matches (juce::AudioProcessorValueTreeState& state, int module, const Preset& preset)
        {
            const auto pre = prefix (module);
            for (int p = info (module).first + 1; p <= info (module).last; ++p)
                if (auto* param = state.getParameter (paramId ((Param) p)))
                {
                    const auto it = preset.values.find (param->getParameterID().substring (pre.length()));
                    const float target = it != preset.values.end() ? param->convertTo0to1 (it->second) : param->getDefaultValue();
                    if (std::abs (param->getValue() - target) > 1.0e-3f)
                        return false;
                }
            return true;
        }

        // Saves the current settings of one effect as a user preset.
        bool saveUserPreset (juce::AudioProcessorValueTreeState& state, int module, const juce::String& name)
        {
            const auto clean = name.trim().isEmpty() ? juce::String ("My Preset") : name.trim();
            const auto pre = prefix (module);

            juce::XmlElement root ("FjordFxPresets");
            root.setAttribute ("effect", info (module).id);
            auto* preset = root.createNewChildElement ("Preset");
            preset->setAttribute ("name", clean);
            preset->setAttribute ("category", "User");
            for (int p = info (module).first + 1; p <= info (module).last; ++p)
                if (auto* param = state.getParameter (paramId ((Param) p)))
                {
                    auto* e = preset->createNewChildElement ("Param");
                    e->setAttribute ("id", param->getParameterID().substring (pre.length()));
                    e->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
                }

            auto folder = getUserFolder (module);
            if (! folder.createDirectory() || ! root.writeTo (folder.getChildFile (juce::File::createLegalFileName (clean) + ".fjordmod")))
                return false;
            loaded[(size_t) module] = false;   // reload on next access
            return true;
        }

        void reload() { loaded.fill (false); }

    private:
        void load (int module)
        {
            auto& list = presets[(size_t) module];
            list.clear();
            const auto fileName = juce::String (info (module).id) + ".fjordmod";
            for (int i = 0; i < FjordBaseData::namedResourceListSize; ++i)
                if (juce::String (FjordBaseData::originalFilenames[i]) == fileName)
                {
                    int size = 0;
                    if (auto* data = FjordBaseData::getNamedResource (FjordBaseData::namedResourceList[i], size))
                        parse (juce::String::fromUTF8 (data, size), {}, list);
                }

            if (! ignoreUserPresets)
                for (const auto& f : getUserFolder (module).findChildFiles (juce::File::findFiles, false, "*.fjordmod"))
                    parse (f.loadFileAsString(), f, list);
            loaded[(size_t) module] = true;
        }

        static void parse (const juce::String& text, const juce::File& file, std::vector<Preset>& out)
        {
            auto xml = juce::parseXML (text);
            if (xml == nullptr) return;
            for (auto* e : xml->getChildWithTagNameIterator ("Preset"))
            {
                Preset p;
                p.name = e->getStringAttribute ("name", "Untitled");
                p.category = file == juce::File() ? e->getStringAttribute ("category", "Factory") : juce::String ("User");
                p.file = file;
                for (auto* v : e->getChildWithTagNameIterator ("Param"))
                    p.values[v->getStringAttribute ("id")] = (float) v->getDoubleAttribute ("value");
                out.push_back (std::move (p));
            }
        }

        std::array<std::vector<Preset>, kNumModules> presets;
        std::array<bool, kNumModules> loaded {};
    };
}
