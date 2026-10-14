#pragma once
#include <fjord_base/presets/PresetManagerBase.h>
#include "Locks.h"

namespace @name@
{
    // Presets are XML files (.@name@) with plain values:
    //   <@Name@Preset name="Soft Saw" category="Basics" author="Fjord" tags="Warm" macros="..." fxOrder="...">
    //     <Param id="osc_wave" value="2"/> …
    // Missing parameters get their default value – this keeps old presets compatible with new versions.
    // Plugin-specific data (envelopes, patterns ...) goes into applyExtras / writeExtras.
    class PresetManager : public fjord::base::presets::PresetManagerBase
    {
    public:
        PresetManager (juce::AudioProcessorValueTreeState& s, juce::UndoManager* um)
            : PresetManagerBase (s, um, { ".@name@", "@Name@Preset", false, {}, { "master_gain" } })
        {
            setLocks (locks::set());   // locked parts (Locks.h) keep their values when a preset is loaded
            initialise();
        }
    };
}
