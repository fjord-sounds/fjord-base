#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <fjord_base/core/Dsp.h>
#include "../Parameters.h"

namespace @name@::dsp
{
    using namespace fjord::base::dsp;

    // A small polyphonic synth as a starting point: one band-limited oscillator, a resonant low pass and an
    // attack/release envelope per voice. Replace it with the plugin's own engine.
    class Synth
    {
    public:
        static constexpr int kNumVoices = 8;

        void prepare (double sr) { sampleRate = (float) sr; reset(); }
        void reset() { for (auto& v : voices) v = {}; }

        void noteOn (int note, float velocity)
        {
            Voice* target = &voices[0];
            for (auto& v : voices)
                if (! v.active) { target = &v; break; }
                else if (v.age > target->age) target = &v;
            *target = {};
            target->active = true;
            target->gate = true;
            target->note = note;
            target->velocity = velocity;
            target->freq = (float) juce::MidiMessage::getMidiNoteInHertz (note);
            ++noteCount;
        }

        void noteOff (int note)
        {
            for (auto& v : voices)
                if (v.active && v.gate && v.note == note) v.gate = false;
        }

        void allNotesOff() { for (auto& v : voices) v.gate = false; }

        // Adds the voices to l / r (n samples)
        void render (float* l, float* r, int n, const params::Values& p)
        {
            namespace P = params;
            const int wave = p.i (P::Wave);
            const float attack = 1.0f / std::max (1.0f, p[P::Attack] * sampleRate);
            const float release = std::exp (-6.9f / std::max (1.0f, p[P::Release] * sampleRate));
            for (auto& v : voices)
            {
                if (! v.active) continue;
                const float dt = v.freq / sampleRate;
                for (int i = 0; i < n; ++i)
                {
                    v.env = v.gate ? std::min (1.0f, v.env + attack) : v.env * release;
                    float s = 0.0f;
                    if (wave == 0)      s = std::sin (2.0f * kPi * v.phase);
                    else if (wave == 1) s = 4.0f * std::abs (v.phase - 0.5f) - 1.0f;
                    else                s = 2.0f * v.phase - 1.0f - polyBlep (v.phase, dt);
                    v.phase += dt;
                    if (v.phase >= 1.0f) v.phase -= 1.0f;
                    s = v.filter.process (s, p[P::Cutoff], p[P::Resonance], sampleRate, Svf::Low);
                    const float out = s * v.env * v.velocity * 0.25f;
                    l[i] += out;
                    r[i] += out;
                }
                ++v.age;
                if (! v.gate && v.env < 1.0e-4f) v.active = false;
            }
        }

        uint32_t getNoteCount() const { return noteCount; }

    private:
        struct Voice
        {
            bool active = false, gate = false;
            int note = 0, age = 0;
            float velocity = 0.0f, freq = 440.0f, phase = 0.0f, env = 0.0f;
            Svf filter;
        };

        std::array<Voice, kNumVoices> voices {};
        float sampleRate = 44100.0f;
        uint32_t noteCount = 0;
    };
}
