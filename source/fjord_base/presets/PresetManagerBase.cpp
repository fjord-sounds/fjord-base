#include "PresetManagerBase.h"
#include "../ui/Brand.h"
#include "BinaryData.h"

namespace fjord::base::presets
{
    namespace
    {
        const juce::Identifier presetNameProp { "presetName" };
        const juce::Identifier presetCategoryProp { "presetCategory" };
        const juce::Identifier macroNamesProp { "macroNames" };

        int categoryRank (const juce::String& c)
        {
            const auto order = PresetManagerBase::categoryOrder();
            const int i = order.indexOf (c);
            return i < 0 ? order.size() : i;
        }
    }

    PresetManagerBase::PresetManagerBase (juce::AudioProcessorValueTreeState& s, juce::UndoManager* um, Config c)
        : state (s), undo (um), config (std::move (c))
    {
    }

    void PresetManagerBase::initialise()
    {
        loadFavorites();
        rescan();
    }

    juce::StringArray PresetManagerBase::categoryOrder()
    {
        auto c = brand().categories;
        c.addIfNotAlreadyThere ("User");
        return c;
    }

    juce::StringArray PresetManagerBase::tagOrder() { return brand().tags; }

    juce::File PresetManagerBase::getUserPresetFolder()
    {
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (brand().userPresetFolder);
    }

    void PresetManagerBase::loadFavorites()
    {
        favorites.clear();
        if (ignoreUserPresets) return;
        favorites.addLines (getUserPresetFolder().getChildFile ("favorites.txt").loadFileAsString());
        favorites.removeEmptyStrings();
    }

    void PresetManagerBase::saveFavorites() const
    {
        auto folder = getUserPresetFolder();
        if (folder.createDirectory())
            folder.getChildFile ("favorites.txt").replaceWithText (favorites.joinIntoString ("\n"));
    }

    bool PresetManagerBase::isFavorite (int index) const
    {
        return juce::isPositiveAndBelow (index, (int) presets.size())
            && favorites.contains (favoriteKey (presets[(size_t) index]));
    }

    void PresetManagerBase::toggleFavorite (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return;

        const auto key = favoriteKey (presets[(size_t) index]);
        if (favorites.contains (key)) favorites.removeString (key);
        else                          favorites.add (key);
        saveFavorites();
        sendChangeMessage();
    }

    bool PresetManagerBase::deleteUserPreset (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()) || presets[(size_t) index].isFactory)
            return false;

        const auto key = favoriteKey (presets[(size_t) index]);
        if (! presets[(size_t) index].file.moveToTrash())
            return false;

        if (favorites.contains (key))
        {
            favorites.removeString (key);
            saveFavorites();
        }
        rescan();
        return true;
    }

    bool PresetManagerBase::parse (const juce::String& xmlText, Preset& out) const
    {
        // Quick scan: read only the root element (name, category, tags). The parameters are parsed on load.
        const int start = xmlText.indexOf ("<" + config.rootTag);
        const int end = start >= 0 ? xmlText.indexOfChar (start, '>') : -1;
        if (start < 0 || end < 0)
            return false;

        auto header = xmlText.substring (start, end);
        if (header.endsWithChar ('/')) header = header.dropLastCharacters (1);
        auto xml = juce::parseXML (header + "/>");
        if (xml == nullptr || ! xml->hasTagName (config.rootTag))
            return false;

        out.name = xml->getStringAttribute ("name", "Untitled");
        out.category = xml->getStringAttribute ("category", "User");
        out.author = xml->getStringAttribute ("author");
        out.description = xml->getStringAttribute ("description");
        out.tags = juce::StringArray::fromTokens (xml->getStringAttribute ("tags"), ",", "");
        out.tags.trim();
        out.tags.removeEmptyStrings();
        out.xml = xmlText;
        return true;
    }

    void PresetManagerBase::collectPresets (std::vector<Preset>& out) const
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            const juce::String file (BinaryData::originalFilenames[i]);
            if (! file.endsWithIgnoreCase (config.extension))
                continue;

            int size = 0;
            if (auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
            {
                Preset p;
                if (parse (juce::String::fromUTF8 (data, size), p))
                    out.push_back (std::move (p));
            }
        }

        if (! ignoreUserPresets)
            for (const auto& f : getUserPresetFolder().findChildFiles (juce::File::findFiles, true, "*" + config.extension))
            {
                Preset p;
                if (parse (f.loadFileAsString(), p))
                {
                    p.file = f;
                    p.isFactory = false;
                    out.push_back (std::move (p));
                }
            }
    }

    void PresetManagerBase::rescan()
    {
        const auto currentName = getCurrentName();
        presets.clear();
        collectPresets (presets);

        std::stable_sort (presets.begin(), presets.end(), [] (const Preset& a, const Preset& b)
        {
            if (a.isFactory != b.isFactory) return a.isFactory;
            const int ra = categoryRank (a.category), rb = categoryRank (b.category);
            if (ra != rb) return ra < rb;
            return a.name.compareNatural (b.name) < 0;
        });

        currentIndex = -1;
        for (size_t i = 0; i < presets.size(); ++i)
            if (presets[i].name == currentName)
                currentIndex = (int) i;

        sendChangeMessage();
    }

    juce::String PresetManagerBase::getCurrentName() const
    {
        return state.state.getProperty (presetNameProp, "Init").toString();
    }

    juce::String PresetManagerBase::getCurrentCategory() const
    {
        return state.state.getProperty (presetCategoryProp, "").toString();
    }

    juce::StringArray PresetManagerBase::getCurrentTags() const
    {
        return juce::isPositiveAndBelow (currentIndex, (int) presets.size()) ? presets[(size_t) currentIndex].tags
                                                                             : juce::StringArray();
    }

    void PresetManagerBase::setCurrent (int index, const juce::String& name, const juce::String& category)
    {
        currentIndex = index;
        state.state.setProperty (presetNameProp, name, nullptr);
        state.state.setProperty (presetCategoryProp, category, nullptr);
        sendChangeMessage();
    }

    juce::StringArray PresetManagerBase::getMacroNames() const
    {
        return juce::StringArray::fromTokens (state.state.getProperty (macroNamesProp).toString(), "|", "");
    }

    void PresetManagerBase::setMacroName (int index, const juce::String& name)
    {
        auto names = getMacroNames();
        while (names.size() <= index) names.add ({});
        names.set (index, name.removeCharacters ("|"));
        state.state.setProperty (macroNamesProp, names.joinIntoString ("|"), undo);
    }

    bool PresetManagerBase::shouldLoadParameter (const juce::String& paramId) const
    {
        if (paramId.startsWith ("fx_") && (bool) state.state.getProperty ("fxLock", false))
            return false;   // "Lock FX": the effect chain stays
        if (lockSet != nullptr && lockSet->isParameterLocked (state.state, paramId))
            return false;   // a locked part of the sound stays
        return ! config.userOwnedParameters.contains (paramId);
    }

    void PresetManagerBase::applyXml (const juce::XmlElement& source, bool isInit)
    {
        juce::XmlElement xml (source);
        migrate (xml);

        std::map<juce::String, float> values;
        for (auto* e : xml.getChildWithTagNameIterator ("Param"))
            values[e->getStringAttribute ("id")] = (float) e->getDoubleAttribute ("value");

        if (undo != nullptr) undo->beginNewTransaction ("Load preset");

        for (auto* p : state.processor.getParameters())
        {
            auto* param = dynamic_cast<juce::RangedAudioParameter*> (p);
            if (param == nullptr || ! shouldLoadParameter (param->getParameterID()))
                continue;
            const auto it = values.find (param->getParameterID());
            const float normalised = it != values.end() ? param->convertTo0to1 (it->second) : param->getDefaultValue();
            param->beginChangeGesture();
            param->setValueNotifyingHost (normalised);
            param->endChangeGesture();
        }

        if (shouldLoadMacros())
            state.state.setProperty (macroNamesProp, xml.getStringAttribute ("macros"), undo);
        if (shouldLoadEffects())
        {
            state.state.setProperty ("fxOrder", normaliseFxOrder (xml.getStringAttribute ("fxOrder")), undo);
            state.state.setProperty ("fxPresetName", juce::String(), nullptr);   // the effects come from the sound preset
        }

        applyExtras (xml, isInit);
    }

    std::unique_ptr<juce::XmlElement> PresetManagerBase::createPresetXml (const juce::String& name, const juce::String& category,
                                                                           const juce::StringArray& tags) const
    {
        auto xml = std::make_unique<juce::XmlElement> (config.rootTag);
        xml->setAttribute ("name", name);
        xml->setAttribute ("category", category);
        xml->setAttribute ("author", "User");
        if (! tags.isEmpty())
            xml->setAttribute ("tags", tags.joinIntoString (","));
        xml->setAttribute ("macros", state.state.getProperty (macroNamesProp).toString());
        xml->setAttribute ("fxOrder", state.state.getProperty ("fxOrder").toString());

        for (auto* p : state.processor.getParameters())
            if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                auto* e = xml->createNewChildElement ("Param");
                e->setAttribute ("id", param->getParameterID());
                e->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
            }
        writeExtras (*xml);
        return xml;
    }

    void PresetManagerBase::load (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return;

        const auto& p = presets[(size_t) index];
        if (auto xml = juce::parseXML (p.xml))
        {
            applyXml (*xml);
            setCurrent (index, p.name, p.category);
        }
    }

    void PresetManagerBase::loadInit()
    {
        applyXml (juce::XmlElement (config.rootTag), true);
        setCurrent (-1, "Init", "");
    }

    void PresetManagerBase::step (int delta)
    {
        if (presets.empty())
            return;

        const int n = (int) presets.size();
        const int start = currentIndex < 0 ? (delta > 0 ? -1 : 0) : currentIndex;
        load (((start + delta) % n + n) % n);
    }

    bool PresetManagerBase::saveUserPreset (const juce::String& name, const juce::String& category, const juce::StringArray& tags)
    {
        const auto cleanName = name.trim().isEmpty() ? juce::String ("Untitled") : name.trim();
        auto xml = createPresetXml (cleanName, category, tags);

        auto folder = getUserPresetFolder().getChildFile (category);
        if (! folder.createDirectory())
            return false;

        auto file = folder.getChildFile (juce::File::createLegalFileName (cleanName) + config.extension);
        if (! xml->writeTo (file))
            return false;

        selectFile (file, cleanName, category);
        return true;
    }

    void PresetManagerBase::selectFile (const juce::File& file, const juce::String& name, const juce::String& category)
    {
        state.state.setProperty (presetNameProp, name, nullptr);
        rescan();
        for (size_t i = 0; i < presets.size(); ++i)
            if (presets[i].file == file)
                setCurrent ((int) i, name, category);
    }

    void PresetManagerBase::restoreFromState()
    {
        const auto name = getCurrentName();
        currentIndex = -1;
        for (size_t i = 0; i < presets.size(); ++i)
            if (presets[i].name == name)
                currentIndex = (int) i;
        sendChangeMessage();
    }
}
