#include "PluginEditor.h"
#include "presets/Locks.h"

using namespace @name@;

ui::ViewSetup @Name@View::makeSetup (@Name@Processor& p)
{
    ui::ViewSetup s { p.apvts, p.presetManager };
    s.undo = &p.undoManager;
    s.masterParamId = params::paramId (params::MasterGain);
    s.tabs = { "Sound", "FX" };
    s.peaks = [&p] { return p.takeOutputPeaks(); };
    s.noteCounter = [&p] { return p.getNoteCount(); };
    s.lastVelocity = [&p] { return p.getLastVelocity(); };
    s.debugReport = [&p] { return p.getDebugReport(); };
    s.panic = [&p] { p.requestPanic(); };
    return s;
}

@Name@View::@Name@View (@Name@Processor& p)
    : PluginView (makeSetup (p), width, height),
      soundPage (p.apvts),
      fxPage ({ p.apvts, &p.undoManager, p.fxMeters, [&p] { return p.fxOrder.get(); },
                [&p] (const fjord::base::fx::Order& o) { p.fxOrder.set (o); }, &p.chainPresets }),
      keyboard (p.keyboardState)
{
    // The padlock in the preset bar: parts of the sound that stay when switching presets
    fjord::base::ui::LockMenu::attach (getPresetBar(), p.apvts, locks::set(), locks::menuLayout());
    addAndMakeVisible (keyboard);
    setPages ({ &soundPage, &fxPage });
}

juce::Rectangle<int> @Name@View::layoutContent (juce::Rectangle<int> r)
{
    keyboard.setBounds (r.removeFromBottom (keyboardHeight + margin).withTrimmedBottom (margin));
    r.removeFromBottom (12);
    r.removeFromTop (4);
    soundPage.setBounds (r);
    fxPage.setBounds (r);
    return r;
}
