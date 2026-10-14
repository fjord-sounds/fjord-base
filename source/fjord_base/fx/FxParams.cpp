#include "FxParams.h"

namespace fjord::base::fx
{
    const std::array<params::Spec, NumParams>& specs()
    {
        static const auto table = params::finish (std::array<params::Spec, NumParams> { FJORD_FX_PARAMS (FJORD_PARAM_SPEC) });
        return table;
    }

    const ModuleInfo& info (int module)
    {
        static const ModuleInfo modules[] = {
            { "saturation", "Saturation", "Satur.",  "Six transfer curves with bias, tilt tone, oversampling and auto gain",   FxSatOn,   FxSatAuto },
            { "lofi",       "Lo-Fi",      "Lo-Fi",   "Tape wow and flutter, age, hiss, bit depth and sample-rate reduction",   FxLofiOn,  FxLofiMix },
            { "transient",  "Transient",  "Trans.",  "More or less attack and sustain, independent of level",                  FxTransOn, FxTransMix },
            { "compressor", "Compressor", "Comp",    "Soft-knee compressor with parallel mix, sidechain filter and auto gain", FxCompOn,  FxCompAuto },
            { "squash",     "Squash",     "Squash",  "Three-band upward and downward compression - quiet details come forward", FxSqOn,  FxSqMix },
            { "eq",         "EQ",         "EQ",      "Low cut, shelves, two bells, high cut  -  drag the points, wheel = Q",   FxEqOn,    FxEqHighSlope },
            { "filter",     "Filter",     "Filter",  "Ladder or state-variable filter with an envelope per note and an LFO",   FxFiltOn,  FxFiltMix },
            { "motion",     "Motion",     "Motion",  "Chorus, ensemble, phaser, flanger, a doubled take or vibrato",           FxMotOn,   FxMotMix },
            { "delay",      "Delay",      "Delay",   "Clean, tape, grain or diffuse echoes, tempo synced, with ducking",        FxDlyOn,   FxDlyMix },
            { "reverb",     "Reverb",     "Reverb",  "Room, plate, hall, shimmer (octave-up tail) or cloud, with freeze",       FxVerbOn,  FxVerbMix },
            { "width",      "Width",      "Width",   "Stereo width, mono below the crossover, Haas spread",                    FxWidthOn, FxWidthHaas },
            { "tremolo",    "Tremolo",    "Tremolo", "Tremolo, auto-pan or harmonic tremolo, tempo synced",                    FxTremOn,  FxTremMix },
            { "shifter",    "Frequency Shifter", "Shifter", "Shifts every frequency by the same amount - from slow beating to bells", FxShiftOn, FxShiftMix },
            { "exciter",    "Exciter",    "Exciter", "Adds harmonics above a frequency for presence and air",                  FxExcOn,   FxExcAmount },
            { "harmonizer", "Harmonizer", "Harmon.", "Two pitched voices with detune, delay and feedback",                     FxHarmOn,  FxHarmMix },
            { "out",        "Clipper",    "Out",     "Always last: soft, hard or tape clipping up to the ceiling",             FxOutOn,   FxOutCeiling } };
        static_assert (std::size (modules) == (size_t) kNumModules);
        return modules[juce::jlimit (0, kNumModules - 1, module)];
    }

    int moduleOf (Param p)
    {
        for (int m = 0; m < kNumModules; ++m)
            if (p >= info (m).first && p <= info (m).last)
                return m;
        return -1;
    }

    int moduleFromId (const juce::String& id)
    {
        for (int m = 0; m < kNumModules; ++m)
            if (id == info (m).id)
                return m;
        return -1;
    }

    int moduleOfParamId (const juce::String& paramId)
    {
        if (! paramId.startsWith ("fx_")) return -1;
        for (int m = 0; m < kNumModules; ++m)
            if (paramId.startsWith ("fx_" + juce::String (info (m).id) + "_")) return m;
        return -1;
    }

    const std::array<float, NumParams>& defaults()
    {
        static const auto values = []
        {
            std::array<float, NumParams> v {};
            for (int i = 0; i < NumParams; ++i)
                v[(size_t) i] = specs()[(size_t) i].def;
            return v;
        }();
        return values;
    }

    double noteBeats (int index)
    {
        static const double beats[] = { 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.375, 0.5, 2.0 / 3.0, 0.75, 1.0,
                                        4.0 / 3.0, 1.5, 2.0, 3.0, 4.0, 8.0 };
        return beats[std::clamp (index, 0, (int) std::size (beats) - 1)];
    }

    // Colour first, then dynamics, then movement and space.
    Order defaultOrder()
    {
        using M = Module;
        return { (int) M::Saturation, (int) M::Transient, (int) M::Filter, (int) M::Eq, (int) M::Compressor, (int) M::Squash,
                 (int) M::Exciter, (int) M::Lofi, (int) M::Harmonizer, (int) M::Shifter, (int) M::Motion, (int) M::Tremolo,
                 (int) M::Delay, (int) M::Reverb, (int) M::Width };
    }

    juce::String orderToString (const Order& o)
    {
        juce::StringArray s;
        for (int m : o) s.add (info (m).id);
        return s.joinIntoString (",");
    }

    Order orderFromString (const juce::String& text, const Order& fallback)
    {
        std::vector<int> list;
        std::array<bool, kNumModules> used {};
        for (const auto& t : juce::StringArray::fromTokens (text, ",", ""))
        {
            const int m = moduleFromId (t.trim());
            if (m >= 0 && m != kOut && ! used[(size_t) m])
            {
                list.push_back (m);
                used[(size_t) m] = true;
            }
        }
        // Missing modules go after the module that precedes them in the fallback order
        for (size_t k = 0; k < fallback.size(); ++k)
        {
            const int m = fallback[k];
            if (used[(size_t) m]) continue;
            auto pos = list.begin();
            for (size_t j = k; j-- > 0;)
                if (auto it = std::find (list.begin(), list.end(), fallback[j]); it != list.end()) { pos = it + 1; break; }
            list.insert (pos, m);
            used[(size_t) m] = true;
        }
        Order o {};
        for (size_t i = 0; i < o.size(); ++i) o[i] = list[i];
        return o;
    }
}
