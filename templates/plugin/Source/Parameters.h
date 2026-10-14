#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <fjord_base/params/Params.h>
#include <fjord_base/fx/FxParams.h>

namespace @name@::params
{
    // Every parameter is listed once here. The columns are:
    //   enum name, parameter ID, display name, kind (F float, I int, C choice, B bool), min, max, default,
    //   curve (0 linear, -1 logarithmic, > 0 skewed so that this value sits in the middle), value format, choices
    //
    // Rules that keep projects and presets working:
    //   - IDs never change. New parameters go to the END of the list.
    //   - Choice lists are append-only.
    //   - New parameters need neutral defaults (presets without them fall back to the default).
    //   - The shared effect block (FJORD_FX_PARAMS) stays complete and in one piece.

    #define @NAME@_PARAMS(X)                                                                                         \
        /* Global */                                                                                                 \
        X (MasterGain,  "master_gain",  "Master",       F, -36, 6, -3,      0, Db, "")                               \
        X (PresetLevel, "preset_level", "Preset Level", F, -24, 12, 0,      0, Db, "")                               \
        /* Sound */                                                                                                  \
        X (Wave,        "osc_wave",     "Wave",         C, 0, 2, 1,         0, None, "Sine|Triangle|Saw")            \
        X (Cutoff,      "filter_cutoff","Cutoff",       F, 40, 18000, 2400, -1, Hz, "")                              \
        X (Resonance,   "filter_reso",  "Resonance",    F, 0, 1, 0.2f,      0, Pct, "")                              \
        X (Attack,      "amp_attack",   "Attack",       F, 0.001f, 4, 0.01f, 0.2f, Time, "")                          \
        X (Release,     "amp_release",  "Release",      F, 0.01f, 8, 0.6f,  0.8f, Time, "")                           \
        /* Effects (shared with every Fjord plugin) */                                                               \
        FJORD_FX_PARAMS (X)

    enum Id : int { @NAME@_PARAMS (FJORD_PARAM_ENUM) Count };

    using Kind = fjord::base::params::Kind;
    using Format = fjord::base::params::Format;
    using Spec = fjord::base::params::Spec;

    const std::array<Spec, Count>& specs();
    inline const Spec& spec (Id id) { return specs()[(size_t) id]; }
    inline juce::String paramId (Id id) { return spec (id).paramId; }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Plain values of all parameters, read once per audio block.
    using Values = fjord::base::params::ValuesT<Id, Count>;

    class Reader : public fjord::base::params::ReaderT<Id, Count>
    {
    public:
        explicit Reader (juce::AudioProcessorValueTreeState& state) : ReaderT (state, specs()) {}
    };

    // The effect block inside the values (fjord-base effects read it directly)
    inline fjord::base::fx::Values fxValues (const Values& v) { return { v.data() + FxSatOn }; }
    static_assert (FxOutCeiling - FxSatOn + 1 == fjord::base::fx::NumParams, "the effect block must be complete and in order");
}
