#pragma once
#include "PluginProcessor.h"
#include "ui/SoundPage.h"
#include <fjord_base/fx/FxPage.h>

// The actual UI at a fixed size (1100 x 740) inside the shared frame of fjord-base (header, presets, About).
// The editor scales it as a whole.
class @Name@View : public @name@::ui::PluginView
{
public:
    static constexpr int width = 1100, height = 740;

    explicit @Name@View (@Name@Processor&);

    // For the screenshot tool (0 Sound, 1 FX)
    void selectPage (int index) { showPage (index); }
    fjord::base::fx::FxPage& getFxPage() { return fxPage; }

protected:
    juce::Rectangle<int> layoutContent (juce::Rectangle<int>) override;

private:
    static @name@::ui::ViewSetup makeSetup (@Name@Processor&);

    @name@::ui::SoundPage soundPage;
    fjord::base::fx::FxPage fxPage;
    @name@::ui::Keyboard keyboard;

    static constexpr int keyboardHeight = 84;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (@Name@View)
};

// Plugin window: freely scalable (75 - 150 %) with a fixed aspect ratio, the size is remembered.
class @Name@Editor : public @name@::ui::ScaledEditor<@Name@View, @Name@Processor>
{
public:
    using ScaledEditor::ScaledEditor;
};
