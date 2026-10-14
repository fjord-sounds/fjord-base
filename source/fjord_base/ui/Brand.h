#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace fjord::base
{
    // All colours of one theme. Every plugin brings its own light and dark palette (see Brand).
    struct Palette
    {
        juce::Colour background, panel, line, text, textDim, accent, accentSoft, track, keyWhite, keyBlack;
    };

    // The family palettes (icy white / polar night) with a given accent. Plugins start from these and may change
    // any colour – the family look comes from the shared layout and typography, not from fixed colours.
    inline Palette familyLight (juce::Colour accent)
    {
        return { juce::Colour (0xffedf3f4),   // background: icy white
                 juce::Colour (0xfff8fbfc),   // panel
                 juce::Colour (0xffcfdce1),   // hairlines
                 juce::Colour (0xff15253a),   // text: deep night blue
                 juce::Colour (0xff6f8291),   // secondary text
                 accent,                      // accent (deep)
                 accent.withAlpha ((juce::uint8) 0x29),
                 juce::Colour (0xffdde7eb),   // knob track
                 juce::Colour (0xfffafcfd),   // white keys
                 juce::Colour (0xff1c2b3d) }; // black keys
    }

    inline Palette familyDark (juce::Colour accent)
    {
        return { juce::Colour (0xff0d1520),   // background: polar night
                 juce::Colour (0xff131e2c),
                 juce::Colour (0xff22314a),
                 juce::Colour (0xffe2edf0),   // text: icy white
                 juce::Colour (0xff7d8fa3),
                 accent,                      // accent (glowing)
                 accent.withAlpha ((juce::uint8) 0x33),
                 juce::Colour (0xff1f2d40),
                 juce::Colour (0xffc3d0d8),
                 juce::Colour (0xff0a1018) };
    }

    // Everything that makes one plugin look like itself inside the shared design: name, colours, folders.
    // Every plugin defines fjord::base::brand() once (e.g. in its PluginProcessor.cpp).
    struct Brand
    {
        juce::String name;              // "Torden" – product name in dialogs and the debug report
        juce::String logo;              // "torden" – lower case logo in the header and the About panel
        juce::String tagline;           // "KICK DESIGNER" – below the logo in the About panel
        juce::String githubUrl;         // https://github.com/fjord-sounds/<repo>
        juce::String settingsFolder;    // "Fjord/Torden" – below the OS settings folder
        juce::StringArray legacySettingsFolders;   // older settings folders, copied over once
        juce::String userPresetFolder;  // "Fjord/Torden/Presets" – below the user's music folder

        // Complete palettes per theme (usually familyLight / familyDark with the plugin's accent)
        Palette light, dark;

        // Three-colour gradient for meters, banners and visuals (aurora, storm, veil ...), per theme
        std::array<juce::Colour, 3> gradientLight, gradientDark;

        // Soft glow behind the header: three colours and their strength per theme
        std::array<juce::Colour, 3> glow;
        float glowDark = 0.14f, glowLight = 0.09f;

        // Colour of the flash on the level meter for every note / hit (transparent = no flash)
        juce::Colour flash = juce::Colours::transparentBlack;

        // Preset categories and tags in display order, and nicer labels for some categories
        juce::StringArray categories, tags;
        std::vector<std::pair<juce::String, juce::String>> categoryLabels;
    };

    const Brand& brand();

    // Category labels for the UI (internally the short names are kept)
    inline juce::String categoryLabel (const juce::String& c)
    {
        for (const auto& [key, label] : brand().categoryLabels)
            if (key == c) return label;
        return c;
    }
}
