#include "Params.h"

namespace fjord::base::params
{
    juce::String trimNumber (float v, int decimals)
    {
        auto s = juce::String (v, decimals);
        if (s.containsChar ('.'))
            s = s.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
        return s == "-0" ? juce::String ("0") : s;
    }

    namespace
    {
        // Bipolar ranges (-x .. +x) with a curve: finer around zero, the curve value sits at 3/4 of the travel.
        bool isSymmetric (const Spec& s) { return s.kind == Kind::Float && s.curve > 0.0f && s.min < 0.0f && std::abs (s.min + s.max) < 1.0e-6f; }
        float symmetricPower (const Spec& s) { return std::log (s.curve / s.max) / std::log (0.5f); }
    }

    float toNormalised (const Spec& s, float v)
    {
        v = std::clamp (v, s.min, s.max);
        if (s.max <= s.min) return 0.0f;
        if (isSymmetric (s))
        {
            const float a = std::pow (std::abs (v) / s.max, 1.0f / symmetricPower (s));
            return 0.5f + 0.5f * (v < 0.0f ? -a : a);
        }
        if (s.curve < 0.0f)
            return std::log (v / s.min) / std::log (s.max / s.min);
        const float lin = (v - s.min) / (s.max - s.min);
        if (s.curve > 0.0f && s.kind == Kind::Float)
        {
            const float skew = std::log (0.5f) / std::log ((s.curve - s.min) / (s.max - s.min));
            return std::pow (lin, skew);
        }
        return lin;
    }

    float fromNormalised (const Spec& s, float n)
    {
        n = std::clamp (n, 0.0f, 1.0f);
        if (isSymmetric (s))
        {
            const float x = 2.0f * n - 1.0f;
            const float a = s.max * std::pow (std::abs (x), symmetricPower (s));
            return x < 0.0f ? -a : a;
        }
        if (s.curve < 0.0f)
            return s.min * std::pow (s.max / s.min, n);
        if (s.curve > 0.0f && s.kind == Kind::Float)
        {
            const float skew = std::log (0.5f) / std::log ((s.curve - s.min) / (s.max - s.min));
            return s.min + (s.max - s.min) * std::pow (n, 1.0f / skew);
        }
        const float v = s.min + (s.max - s.min) * n;
        return s.kind == Kind::Float ? v : std::round (v);
    }

    juce::String noteName (int note)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (names[((note % 12) + 12) % 12]) + juce::String ((int) std::floor ((float) note / 12.0f) - 2);
    }

    juce::String formatValue (const Spec& s, float v)
    {
        using enum Format;
        switch (s.format)
        {
            case Pct:   return juce::String (juce::roundToInt (v * 100.0f)) + " %";
            case Bip:   return (v > 0.005f ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + " %";
            case Hz:
            {
                const float a = std::abs (v);
                const auto sign = juce::String (v < 0.0f ? "-" : (s.min < 0.0f && v > 0.0f ? "+" : ""));
                return a < 1000.0f ? sign + trimNumber (a, a < 10.0f ? 2 : (a < 100.0f ? 1 : 0)) + " Hz"
                                   : sign + trimNumber (a / 1000.0f, 2) + " kHz";
            }
            case Time:  return v < 1.0f ? trimNumber (v * 1000.0f, v < 0.1f ? 1 : 0) + " ms" : trimNumber (v, 2) + " s";
            case Ms:    return v < 10.0f ? trimNumber (v, 2) + " ms" : (v < 1000.0f ? trimNumber (v, 0) + " ms" : trimNumber (v / 1000.0f, 2) + " s");
            case Db:    return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB";
            case Semi:  return (v > 0.05f ? "+" : "") + juce::String (v, s.kind == Kind::Float ? 1 : 0) + " st";
            case Cents: return (v > 0.5f ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
            case Mult:  return trimNumber (v, 2) + juce::String::fromUTF8 (" \xc3\x97");
            case Deg:   return juce::String (juce::roundToInt (v)) + juce::String::fromUTF8 ("\xc2\xb0");
            case Oct:   return (v > 0.005f && s.min < 0.0f ? "+" : "") + trimNumber (v, s.kind == Kind::Int ? 0 : 1) + " oct";
            case Bits:  return trimNumber (v, 1) + " bit";
            case Ratio: return trimNumber (v, 1) + " : 1";
            case Bpm:   return juce::String (v, 1) + " BPM";
            case Steps: return juce::String (juce::roundToInt (v)) + " steps";
            case Note:  return noteName (juce::roundToInt (v));
            case Speed: return (std::abs (v) < 0.005f ? juce::String ("frozen")
                                                      : (v > 0.0f ? "" : "-") + trimNumber (std::abs (v), 2) + juce::String::fromUTF8 (" \xc3\x97"));
            case Rate:  return trimNumber (v, v < 10.0f ? 1 : 0) + " / s";
            case Pan:
            {
                const int p = juce::roundToInt (v * 100.0f);
                return p == 0 ? juce::String ("C") : (p < 0 ? "L" + juce::String (-p) : "R" + juce::String (p));
            }
            case Shape:
            {
                static const char* names[] = { "Sine", "Triangle", "Saw", "Square" };
                const int a = std::clamp ((int) std::floor (v), 0, 3);
                const float f = v - (float) a;
                if (f < 0.02f || a == 3) return names[a];
                if (f > 0.98f) return names[a + 1];
                return juce::String (names[a]) + " > " + names[a + 1] + " " + juce::String (juce::roundToInt (f * 100.0f)) + "%";
            }
            case Vowel:
            {
                static const char* names[] = { "A", "E", "I", "O", "U" };
                const int a = std::clamp ((int) std::floor (v), 0, 4);
                const float f = v - (float) a;
                if (f < 0.05f || a == 4) return names[a];
                if (f > 0.95f) return names[a + 1];
                return juce::String (names[a]) + " > " + names[a + 1];
            }
            case Plain:
            case None:
            default:    return trimNumber (v, 2);
        }
    }

    std::unique_ptr<juce::RangedAudioParameter> createParameter (const Spec& s)
    {
        const juce::ParameterID pid { s.paramId, 1 };
        switch (s.kind)
        {
            case Kind::Float:
            {
                const auto copy = s;
                juce::NormalisableRange<float> range (
                    s.min, s.max,
                    [copy] (float, float, float n) { return fromNormalised (copy, n); },
                    [copy] (float, float, float v) { return toNormalised (copy, v); });
                return std::make_unique<juce::AudioParameterFloat> (
                    pid, s.name, range, s.def,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                        [copy] (float v, int) { return formatValue (copy, v); }));
            }
            case Kind::Int:
            {
                const auto copy = s;
                return std::make_unique<juce::AudioParameterInt> (
                    pid, s.name, (int) s.min, (int) s.max, (int) s.def,
                    juce::AudioParameterIntAttributes().withStringFromValueFunction (
                        [copy] (int v, int) { return formatValue (copy, (float) v); }));
            }
            case Kind::Choice:
                return std::make_unique<juce::AudioParameterChoice> (pid, s.name, s.choices, (int) s.def);
            case Kind::Bool:
            default:
                return std::make_unique<juce::AudioParameterChoice> (pid, s.name, juce::StringArray { "Off", "On" }, s.def > 0.5f ? 1 : 0);
        }
    }
}
