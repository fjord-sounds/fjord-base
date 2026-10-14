#pragma once
#include "Locks.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace fjord::base::presets
{
    // Sound presets: XML files with plain parameter values, e.g.
    //   <TordenPreset name="Deep Stone" category="Techno" author="Fjord" tags="Deep,Sub" macros="..." fxOrder="...">
    //     <Param id="body_level" value="0.8"/> … plugin-specific children (envelopes, patterns …)
    // Factory presets are embedded (BinaryData, files with the plugin's extension), user presets live in
    // ~/Music/<brand().userPresetFolder>/<Category>/<Name>.<ext>. Missing parameters get their default value – this keeps
    // old presets working with new versions. Favorites are stored in favorites.txt in the user folder.
    //
    // A plugin derives from this class and overrides the hooks for what it stores besides parameters.
    class PresetManagerBase : public juce::ChangeBroadcaster
    {
    public:
        struct Preset
        {
            juce::String name, category, author, description;
            juce::StringArray tags;
            juce::String xml;          // file contents
            juce::File file;           // empty for factory presets
            bool isFactory = true;
        };

        struct Config
        {
            juce::String extension;          // ".torden"
            juce::String rootTag;            // "TordenPreset"
            bool hasPatterns = false;        // offers "load patterns with presets" (step / motion patterns)
            juce::String patternLabel { "Load patterns with presets" };
            juce::StringArray userOwnedParameters { "master_gain" };   // never loaded from presets
        };

        // Screenshots: show only the factory presets, never the user's own presets or favourites.
        static inline bool ignoreUserPresets = false;

        PresetManagerBase (juce::AudioProcessorValueTreeState&, juce::UndoManager*, Config);
        ~PresetManagerBase() override = default;

        // Call once at the end of the derived constructor (scans factory and user presets).
        void initialise();

        const Config& getConfig() const { return config; }
        static juce::StringArray categoryOrder();   // brand().categories + "User"
        static juce::StringArray tagOrder();        // brand().tags

        void rescan();
        const std::vector<Preset>& getPresets() const { return presets; }

        int getCurrentIndex() const { return currentIndex; }
        juce::String getCurrentName() const;
        juce::String getCurrentCategory() const;
        juce::StringArray getCurrentTags() const;

        void load (int index);
        void loadInit();
        void next()     { step (1); }
        void previous() { step (-1); }

        // Saves the current sound as a user preset and selects it.
        virtual bool saveUserPreset (const juce::String& name, const juce::String& category, const juce::StringArray& tags = {});

        static juce::File getUserPresetFolder();

        bool isFavorite (int index) const;
        void toggleFavorite (int index);

        // Only user presets can be deleted.
        bool deleteUserPreset (int index);

        juce::ValueTree& getState() { return state.state; }
        juce::AudioProcessorValueTreeState& getValueTreeState() { return state; }
        juce::UndoManager* getUndoManager() const { return undo; }

        // When off, loading a preset keeps the current pattern (stored as "loadPatterns" in the state).
        bool getLoadPatterns() const { return state.state.getProperty ("loadPatterns", true); }
        void setLoadPatterns (bool shouldLoad) { state.state.setProperty ("loadPatterns", shouldLoad, nullptr); }

        // Macro names ("Punch|Length|…") stored in the state
        juce::StringArray getMacroNames() const;
        void setMacroName (int index, const juce::String& name);

        // Writes the current sound as preset XML (also used for A/B snapshots and exports).
        std::unique_ptr<juce::XmlElement> createPresetXml (const juce::String& name, const juce::String& category,
                                                           const juce::StringArray& tags = {}) const;
        // Applies preset XML (parameters, macro names, effect order and the plugin's own data).
        void applyXml (const juce::XmlElement&, bool isInit = false);

        // For host recall: keep the name of the active preset in the state.
        void restoreFromState();

        // The plugin's lock groups (see Locks.h): locked parts keep their values when a preset is loaded. Set once by the
        // plugin; the set must outlive the manager (a static).
        void setLocks (const locks::LockSet& set) { lockSet = &set; }
        const locks::LockSet* getLocks() const { return lockSet; }
        // For applyExtras: is the part with this lock property locked (envelopes, patterns, key edits …)?
        bool isLockedProperty (const juce::String& property) const { return locks::LockSet::isLockedProperty (state.state, property); }

    protected:
        // Collects factory and user presets (default: BinaryData files and the user folder with the extension).
        virtual void collectPresets (std::vector<Preset>&) const;
        // Selects a preset after saving (by its file).
        void selectFile (const juce::File&, const juce::String& name, const juce::String& category);
        bool parse (const juce::String& xmlText, Preset& out) const;

        // Converts old preset XML in place (renamed parameters, old formats). Called before every apply.
        virtual void migrate (juce::XmlElement&) const {}
        // Whether a parameter is set from the preset (false: it keeps its value, e.g. locked parts).
        virtual bool shouldLoadParameter (const juce::String& paramId) const;
        // Plugin data besides the parameters. 'isInit': the empty init preset (restore defaults).
        virtual void applyExtras (const juce::XmlElement&, bool isInit) { juce::ignoreUnused (isInit); }
        virtual void writeExtras (juce::XmlElement&) const {}
        // Whether the macro names / the effects come from the preset ("Lock FX" keeps the effect chain).
        virtual bool shouldLoadMacros() const { return ! isLockedProperty ("macroLock"); }
        virtual bool shouldLoadEffects() const { return ! (bool) state.state.getProperty ("fxLock", false); }
        // Normalises an effect order string from a preset (default: kept as it is).
        virtual juce::String normaliseFxOrder (const juce::String& order) const { return order; }

        juce::AudioProcessorValueTreeState& state;
        juce::UndoManager* undo = nullptr;

    private:
        void step (int delta);
        void setCurrent (int index, const juce::String& name, const juce::String& category);
        static juce::String favoriteKey (const Preset& p) { return (p.isFactory ? "factory/" : "user/") + p.category + "/" + p.name; }
        void loadFavorites();
        void saveFavorites() const;

        Config config;
        const locks::LockSet* lockSet = nullptr;
        std::vector<Preset> presets;
        int currentIndex = -1;
        juce::StringArray favorites;
    };
}
