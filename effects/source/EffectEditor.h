#pragma once
#include "EffectProcessor.h"
#include <fjord_base/fx/FxPage.h>

namespace fjord::effects
{
    // The window of a single-effect plugin: the shared header (logo, meter, presets, theme, output level), the big
    // graph of the effect on the left and its controls on the right.
    class EffectView : public fjord::base::ui::PluginView
    {
    public:
        static constexpr int width = 1040, height = 600;

        explicit EffectView (EffectProcessor&);

        void tickContent (bool offscreen) override;

    protected:
        juce::Rectangle<int> layoutContent (juce::Rectangle<int> area) override;

    private:
        static fjord::base::ui::ViewSetup makeSetup (EffectProcessor&);

        EffectProcessor& proc;
        fx::Host host;
        fx::UiValues uiValues;
        fx::Anim anim;
        fx::GraphPanel graph;
        fx::ModuleEditor editor;
    };

    class EffectEditor : public fjord::base::ui::ScaledEditor<EffectView, EffectProcessor>
    {
    public:
        using ScaledEditor::ScaledEditor;
    };
}
