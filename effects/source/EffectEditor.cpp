#include "EffectEditor.h"

namespace fjord::effects
{
    fjord::base::ui::ViewSetup EffectView::makeSetup (EffectProcessor& p)
    {
        fjord::base::ui::ViewSetup s { p.apvts, p.presetManager };
        s.undo = &p.undoManager;
        s.peaks = [&p] { return p.outputPeaks.take(); };
        s.debugReport = [&p] { return p.getDebugReport(); };
        s.panic = [&p] { p.requestReset(); };
        s.rememberPage = false;
        s.meterWidth = 120;
        s.masterLabel = "Output";
        return s;
    }

    EffectView::EffectView (EffectProcessor& p)
        : PluginView (makeSetup (p), width, height),
          proc (p),
          host { p.apvts, &p.undoManager, p.meters, nullptr, nullptr },
          uiValues (p.apvts),
          graph (host, anim),
          editor (host)
    {
        graph.setType (p.module);
        editor.select (p.module);
        // One fixed page (graph + controls) instead of tabs; the overlays stay on top (setPages brings them forward)
        addAndMakeVisible (graph);
        addAndMakeVisible (editor);
        setPages ({});
    }

    void EffectView::tickContent (bool)
    {
        uiValues.update();
        const auto v = uiValues.view();
        anim.tick (proc.meters, v, 1.0f / 30.0f, proc.module);
        graph.setValues (v);
    }

    juce::Rectangle<int> EffectView::layoutContent (juce::Rectangle<int> area)
    {
        area.removeFromBottom (margin);
        graph.setBounds (area.removeFromLeft ((int) ((float) area.getWidth() * 0.46f)));
        area.removeFromLeft (12);
        editor.setBounds (area);
        return area.withBottom (getHeight() - margin);
    }
}
