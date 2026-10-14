#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// Parameter tables: every plugin lists its parameters once in an X-macro table with these columns
//   enum name, parameter ID, display name, kind (F float, I int, C choice, B bool), min, max, default,
//   curve (0 linear, -1 logarithmic, > 0 skewed so that this value sits in the middle; for -x..+x ranges it sits at
//   3/4 of the travel, finer around zero), value format, choices ("a|b|c")
// and builds its specs with FJORD_PARAM_SPEC. This file has everything that works on such a table: value <-> 0..1,
// display text, JUCE parameters, plain-value snapshots for the audio thread.
namespace fjord::base::params
{
    enum class Kind { Float, Int, Choice, Bool };

    constexpr Kind F = Kind::Float;
    constexpr Kind I = Kind::Int;
    constexpr Kind C = Kind::Choice;
    constexpr Kind B = Kind::Bool;

    enum class Format
    {
        None, Pct, Bip, Hz, Time, Ms, Db, Semi, Cents, Pan, Mult, Deg, Oct, Bits, Ratio, Shape, Note, Bpm, Steps, Plain,
        Speed, Rate, Vowel,
        Balance = Pan
    };

    struct Spec
    {
        int id = 0;
        const char* paramId = "";
        const char* name = "";
        Kind kind = Kind::Float;
        float min = 0.0f, max = 1.0f, def = 0.0f, curve = 0.0f;
        Format format = Format::None;
        juce::StringArray choices;
    };

    // Mapping between plain values and 0..1 (the same maths the host sees)
    float toNormalised (const Spec&, float plain);
    float fromNormalised (const Spec&, float normalised);

    // Note names with C3 = MIDI 60 (as in Logic and Ableton), everywhere in the family.
    juce::String noteName (int midiNote);
    juce::String formatValue (const Spec&, float plain);

    // Bool parameters are two-step choices (Off / On): hosts may send in-between values, and a choice snaps them,
    // so the value a host reads back always matches what is saved.
    std::unique_ptr<juce::RangedAudioParameter> createParameter (const Spec&);

    template <typename Table>
    void addToLayout (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const Table& specs)
    {
        for (const auto& s : specs)
            layout.add (createParameter (s));
    }

    // Plain values of a whole table, read once per audio block.
    template <typename IdType, int N>
    struct ValuesT
    {
        std::array<float, (size_t) N> v {};
        float operator[] (IdType id) const { return v[(size_t) id]; }
        float& operator[] (IdType id) { return v[(size_t) id]; }
        int i (IdType id) const { return (int) std::lround (v[(size_t) id]); }
        bool b (IdType id) const { return v[(size_t) id] > 0.5f; }
        const float* data() const { return v.data(); }
        float* data() { return v.data(); }
    };

    // Reads all raw parameter values of a table (lock-free, audio thread safe).
    template <typename IdType, int N>
    class ReaderT
    {
    public:
        template <typename Table>
        ReaderT (juce::AudioProcessorValueTreeState& state, const Table& specs)
        {
            for (int i = 0; i < N; ++i)
            {
                raw[(size_t) i] = state.getRawParameterValue (specs[(size_t) i].paramId);
                jassert (raw[(size_t) i] != nullptr);
            }
        }

        void read (ValuesT<IdType, N>& out) const
        {
            for (size_t i = 0; i < raw.size(); ++i)
                out.v[i] = raw[i]->load (std::memory_order_relaxed);
        }

    private:
        std::array<std::atomic<float>*, (size_t) N> raw {};
    };

    // Removes empty choice entries and returns the table (call once after building it).
    template <typename Table>
    Table finish (Table table)
    {
        for (auto& s : table)
            s.choices.removeEmptyStrings();
        return table;
    }

    juce::String trimNumber (float v, int decimals);
}

// One table row -> Spec. Use inside a braced initialiser: { MY_PARAMS (FJORD_PARAM_SPEC) }
#define FJORD_PARAM_SPEC(e, pid, nm, kind, lo, hi, def, curve, fmt, ch)                                                   \
    ::fjord::base::params::Spec { (int) (e), pid, nm, ::fjord::base::params::kind, (float) (lo), (float) (hi),            \
                                  (float) (def), (float) (curve), ::fjord::base::params::Format::fmt,                     \
                                  juce::StringArray::fromTokens (ch, "|", "") },

// One table row -> enum entry: enum Id { MY_PARAMS (FJORD_PARAM_ENUM) Count };
#define FJORD_PARAM_ENUM(e, ...) e,
