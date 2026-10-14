#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Brand.h"
#include "Fonts.h"

namespace fjord::base::ui
{
    using fjord::base::Palette;

    // The palettes come completely from the plugin's Brand (usually the family colours with its own accent).
    inline Palette lightPalette() { return brand().light; }
    inline Palette darkPalette()  { return brand().dark; }

    // The plugin's three-colour gradient (aurora, storm, veil ...), darker in the light theme.
    inline const std::array<juce::Colour, 3>& gradientColours (bool dark)
    {
        return dark ? brand().gradientDark : brand().gradientLight;
    }

    inline juce::Colour gradientAt (bool dark, float x)
    {
        const auto& c = gradientColours (dark);
        x = juce::jlimit (0.0f, 1.0f, x);
        return x < 0.5f ? c[0].interpolatedWith (c[1], x * 2.0f) : c[1].interpolatedWith (c[2], x * 2.0f - 1.0f);
    }

    // Soft glow (three blurred discs) behind the header and in backdrops.
    inline void drawGlow (juce::Graphics& g, juce::Rectangle<float> area, bool dark, float strength = 1.0f)
    {
        const float a = (dark ? brand().glowDark : brand().glowLight) * strength;
        for (int i = 0; i < 3; ++i)
        {
            const auto colour = brand().glow[(size_t) i];
            const float cx = area.getX() + area.getWidth() * (0.18f + 0.32f * (float) i);
            const float cy = area.getY() + area.getHeight() * (0.15f + 0.1f * (float) (i % 2));
            juce::ColourGradient grad (colour.withAlpha (a), cx, cy, colour.withAlpha (0.0f),
                                       cx + area.getWidth() * 0.32f, cy, true);
            g.setGradientFill (grad);
            g.fillEllipse (juce::Rectangle<float> (area.getWidth() * 0.64f, area.getHeight() * 1.4f).withCentre ({ cx, cy }));
        }
    }

    // Global theme and per-user settings, stored in the user folder and shared by all instances of the plugin.
    class Theme : public juce::ChangeBroadcaster
    {
    public:
        static Theme& get()
        {
            static Theme instance;
            return instance;
        }

        const Palette& palette() const { return current; }

        // Tests switch saving off so they do not change the user's settings.
        void setPersistent (bool shouldPersist) { persistent = shouldPersist; }
        bool isDark() const { return dark; }

        // Generic per-user flags (reduce motion, last page ...)
        bool getFlag (const juce::String& key, bool def) const
        {
            auto* file = userSettings();
            return file != nullptr ? file->getBoolValue (key, def) : def;
        }

        void setFlag (const juce::String& key, bool v)
        {
            if (auto* file = persistent ? userSettings() : nullptr)
            {
                file->setValue (key, v);
                file->saveIfNeeded();
            }
            sendChangeMessage();
        }

        bool reduceMotion() const { return getFlag ("reduceMotion", false); }

        // Window size (scale) is remembered globally
        float getScale() const
        {
            auto* file = userSettings();
            return file != nullptr ? (float) file->getDoubleValue ("uiScale", 1.0) : 1.0f;
        }

        void setScale (float s)
        {
            if (auto* file = persistent ? userSettings() : nullptr)
            {
                file->setValue ("uiScale", s);
                file->saveIfNeeded();
            }
        }

        juce::File getSettingsFile() const
        {
            auto* file = userSettings();
            return file != nullptr ? file->getFile() : juce::File();
        }

        void setDark (bool shouldBeDark)
        {
            if (dark == shouldBeDark)
                return;
            dark = shouldBeDark;
            current = dark ? darkPalette() : lightPalette();
            if (auto* file = persistent ? userSettings() : nullptr)
            {
                file->setValue ("darkTheme", dark);
                file->saveIfNeeded();   // write immediately – hosts may end the process without cleaning up
            }
            sendChangeMessage();
        }

    private:
        Theme()
        {
            juce::PropertiesFile::Options o;
            o.applicationName = brand().name;
            o.filenameSuffix = ".settings";
            o.folderName = brand().settingsFolder;
            o.osxLibrarySubFolder = "Application Support";
            o.millisecondsBeforeSaving = 0;
            migrateOldSettings (o);
            settings.setStorageParameters (o);

            if (auto* file = settings.getUserSettings())
                dark = file->getBoolValue ("darkTheme", false);
            current = dark ? darkPalette() : lightPalette();
        }

        juce::PropertiesFile* userSettings() const
        {
            return const_cast<juce::ApplicationProperties&> (settings).getUserSettings();
        }

        // Settings that lived in an older folder are copied over once.
        static void migrateOldSettings (juce::PropertiesFile::Options o)
        {
            const auto newFile = o.getDefaultFile();
            if (newFile.existsAsFile())
                return;
            for (const auto& folder : brand().legacySettingsFolders)
            {
                o.folderName = folder;
                const auto oldFile = o.getDefaultFile();
                if (oldFile.existsAsFile())
                {
                    newFile.getParentDirectory().createDirectory();
                    oldFile.copyFileTo (newFile);
                    return;
                }
            }
        }

        juce::ApplicationProperties settings;
        bool dark = false, persistent = true;
        Palette current;
    };

    // String literals with non-ASCII characters are UTF-8 – this displays them correctly.
    inline juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }
}
