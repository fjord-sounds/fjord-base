// Offline test and tool for the single-effect plugins and the shared effect DSP. No DAW needed.
//
//   FjordEffectsTest                                   all tests (must print ALL PASSED)
//   FjordEffectsTest --sr 96000                        the same at another sample rate
//   FjordEffectsTest --render <effect> <preset|init> <out.wav>          a test phrase through one effect
//   FjordEffectsTest --screenshot <effect> <light|dark> <out.png> [preset]
//   FjordEffectsTest --list                            effects and their preset counts
//   FjordEffectsTest --levels <json>                   RMS change of every preset (feeds the preset generator)
//   FjordEffectsTest --strength [json]                 how far every preset moves the sound: level of (processed - dry)
//                                                      relative to the dry phrase in dB (0 dB = as loud as the input;
//                                                      below about -12 dB a preset is hard to hear; measured on a
//                                                      broadband stereo phrase)
#include "EffectProcessor.h"
#include "EffectEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

using namespace fjord::effects;

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        if (! ok)
        {
            ++failures;
            printf ("FAIL  %s\n", what.toRawUTF8());
        }
    }

    // A test phrase: plucked tones, a chord, noise bursts and silence (exercises dynamics, tails and filters)
    juce::AudioBuffer<float> phrase (double sr, double seconds)
    {
        juce::AudioBuffer<float> b (2, (int) (sr * seconds));
        b.clear();
        juce::Random rnd (42);
        const float notes[] = { 220.0f, 330.0f, 277.2f, 440.0f };
        for (int k = 0; k < 4; ++k)
        {
            const int start = (int) (sr * (0.1 + 0.45 * k));
            for (int i = 0; i < (int) (sr * 0.4) && start + i < b.getNumSamples(); ++i)
            {
                const float t = (float) i / (float) sr;
                const float env = std::exp (-t * 6.0f) * std::min (1.0f, t * 400.0f);
                const float s = 0.4f * env * (std::sin (6.2831853f * notes[k] * t) + 0.3f * std::sin (6.2831853f * notes[k] * 2.0f * t));
                b.addSample (0, start + i, s);
                b.addSample (1, start + i, s * 0.9f);
            }
        }
        const int burst = (int) (sr * 2.0);
        for (int i = 0; i < (int) (sr * 0.05) && burst + i < b.getNumSamples(); ++i)
        {
            const float n = 0.3f * (rnd.nextFloat() * 2.0f - 1.0f);
            b.setSample (0, burst + i, n);
            b.setSample (1, burst + i, -n);
        }
        return b;
    }


    // A broadband stereo phrase for judging how strongly a preset acts: detuned saw chords (left and right differ), a
    // noise bed and hi-hat style bursts – there is something to excite, widen, filter and compress.
    juce::AudioBuffer<float> richPhrase (double sr, double seconds)
    {
        juce::AudioBuffer<float> b (2, (int) (sr * seconds));
        b.clear();
        juce::Random rnd (7);
        const float chord[] = { 110.0f, 164.8f, 220.0f, 277.2f, 329.6f };
        std::array<std::array<float, 5>, 2> phase {};
        float lp[2] = { 0.0f, 0.0f };
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) i / (float) sr;
            const float gate = std::fmod (t, 1.2f) < 0.9f ? 1.0f : 0.0f;
            const float env = gate * (0.35f + 0.65f * std::exp (-std::fmod (t, 1.2f) * 3.0f));
            for (int ch = 0; ch < 2; ++ch)
            {
                float s = 0.0f;
                for (int k = 0; k < 5; ++k)
                {
                    const float f = chord[k] * (1.0f + (ch == 0 ? -0.004f : 0.004f) * (float) (k + 1));
                    auto& ph = phase[(size_t) ch][(size_t) k];
                    ph += f / (float) sr;
                    if (ph >= 1.0f) ph -= 1.0f;
                    s += (2.0f * ph - 1.0f) * 0.07f;
                }
                const float noise = rnd.nextFloat() * 2.0f - 1.0f;
                lp[ch] += 0.12f * (noise - lp[ch]);
                s *= env;
                s += 0.02f * lp[ch];
                // hi-hat on every eighth: short high-passed noise burst
                const float beat = std::fmod (t, 0.3f);
                s += beat < 0.04f ? 0.16f * (noise - lp[ch]) * std::exp (-beat * 90.0f) : 0.0f;
                b.setSample (ch, i, s);
            }
        }
        return b;
    }

    struct Result { float peak = 0.0f, rmsIn = 0.0f, rmsOut = 0.0f; bool finite = true; };

    // Processes 'audio' in place
    Result renderInPlace (EffectProcessor& p, double sr, int block, juce::AudioBuffer<float>& audio)
    {
        Result r;
        r.rmsIn = std::max (audio.getRMSLevel (0, 0, audio.getNumSamples()), audio.getRMSLevel (1, 0, audio.getNumSamples()));
        p.setRateAndBufferSizeDetails (sr, block);
        p.prepareToPlay (sr, block);
        juce::MidiBuffer midi;
        for (int start = 0; start < audio.getNumSamples(); start += block)
        {
            const int n = std::min (block, audio.getNumSamples() - start);
            juce::AudioBuffer<float> part (audio.getArrayOfWritePointers(), 2, start, n);
            p.processBlock (part, midi);
        }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
            {
                const float s = audio.getSample (ch, i);
                if (! std::isfinite (s)) r.finite = false;
                r.peak = std::max (r.peak, std::abs (s));
            }
        r.rmsOut = std::max (audio.getRMSLevel (0, 0, audio.getNumSamples()), audio.getRMSLevel (1, 0, audio.getNumSamples()));
        return r;
    }

    Result render (EffectProcessor& p, double sr, int block, juce::AudioBuffer<float> audio) { return renderInPlace (p, sr, block, audio); }

    int moduleByName (const juce::String& id)
    {
        const int m = fx::moduleFromId (id);
        if (m < 0) printf ("unknown effect %s\n", id.toRawUTF8());
        return m;
    }

    bool loadPreset (EffectProcessor& p, const juce::String& name)
    {
        if (name == "init") { p.presetManager.loadInit(); return true; }
        const auto& list = p.presetManager.getPresets();
        for (int i = 0; i < (int) list.size(); ++i)
            if (list[(size_t) i].name == name) { p.presetManager.load (i); return true; }
        printf ("unknown preset %s\n", name.toRawUTF8());
        return false;
    }

    void runTests (double sr)
    {
        for (int m = 0; m < fx::kNumModules; ++m)
        {
            setCurrentModule (m);
            const juce::String id = fx::info (m).id;
            EffectProcessor p (m);

            // Init, three block sizes (hosts may send odd and oversized blocks)
            for (int block : { 64, 441, 4096 })
            {
                p.presetManager.loadInit();
                const auto r = render (p, sr, block, phrase (sr, 4.0));
                check (r.finite, id + " init: finite (block " + juce::String (block) + ")");
                check (r.peak < 4.0f, id + " init: peak " + juce::String (r.peak, 2));
            }

            // Silence in, silence out (after the tail)
            {
                p.presetManager.loadInit();
                juce::AudioBuffer<float> quiet (2, (int) (sr * 2.0));
                quiet.clear();
                const auto r = render (p, sr, 512, quiet);
                check (r.peak < 0.02f, id + ": silence stays quiet (" + juce::String (r.peak, 4) + ")");
            }

            // Every preset
            const auto& list = p.presetManager.getPresets();
            int factory = 0;
            for (int i = 0; i < (int) list.size(); ++i)
            {
                if (! list[(size_t) i].isFactory) continue;
                ++factory;
                p.presetManager.load (i);
                const auto r = render (p, sr, 512, phrase (sr, 3.0));
                const auto name = id + " / " + list[(size_t) i].name;
                check (r.finite, name + ": finite");
                check (r.peak < 4.0f, name + ": peak " + juce::String (r.peak, 2));
                check (r.rmsOut > r.rmsIn * 0.05f, name + ": not silent");
            }
            check (factory >= 40, id + ": at least 40 presets (" + juce::String (factory) + ")");
            printf ("  %-12s %3d presets\n", id.toRawUTF8(), factory);
        }
    }

    int screenshot (int m, bool dark, const juce::File& out, const juce::String& preset)
    {
        setCurrentModule (m);
        fjord::base::ui::Theme::get().setPersistent (false);
        fjord::base::ui::Theme::get().setDark (dark);
        EffectProcessor p (m);
        if (preset.isNotEmpty() && ! loadPreset (p, preset)) return 1;
        EffectView view (p);
        view.setSize (EffectView::width, EffectView::height);
        // Play the test phrase in 1/30 s slices so the meters and graphs animate like in a host
        const auto audio = phrase (48000.0, 2.4);
        const int slice = 1600;
        p.setRateAndBufferSizeDetails (48000.0, 512);
        p.prepareToPlay (48000.0, 512);
        for (int start = 0; start + slice <= audio.getNumSamples(); start += slice)
        {
            juce::AudioBuffer<float> part (2, slice);
            for (int ch = 0; ch < 2; ++ch) part.copyFrom (ch, 0, audio, ch, start, slice);
            juce::MidiBuffer midi;
            p.processBlock (part, midi);
            view.tickAnimations();
        }
        const auto image = view.createComponentSnapshot (view.getLocalBounds(), true, 2.0f);
        out.getParentDirectory().createDirectory();
        juce::FileOutputStream stream (out);
        stream.setPosition (0);
        stream.truncate();
        juce::PNGImageFormat png;
        return png.writeImageToStream (image, stream) ? 0 : 1;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    fjord::base::ui::Theme::get().setPersistent (false);
    fjord::base::presets::PresetManagerBase::ignoreUserPresets = true;
    fx::ModulePresets::ignoreUserPresets = true;

    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (argv[i]);
    auto file = [] (const juce::String& path) { return juce::File::getCurrentWorkingDirectory().getChildFile (path); };

    if (args.size() > 0 && args[0] == "--list")
    {
        for (int m = 0; m < fx::kNumModules; ++m)
            printf ("%-12s %-20s %3d presets\n", fx::info (m).id, fx::info (m).name, (int) fx::ModulePresets::get().getPresets (m).size());
        return 0;
    }

    if (args.size() > 3 && args[0] == "--render")
    {
        const int m = moduleByName (args[1]);
        if (m < 0) return 1;
        setCurrentModule (m);
        EffectProcessor p (m);
        if (! loadPreset (p, args[2])) return 1;
        auto audio = phrase (48000.0, 6.0);
        renderInPlace (p, 48000.0, 512, audio);
        juce::WavAudioFormat wav;
        const auto out = file (args[3]);
        out.deleteFile();
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (out), 48000.0, 2, 24, {}, 0));
        return writer != nullptr && writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()) ? 0 : 1;
    }

    if (args.size() > 3 && args[0] == "--screenshot")
    {
        const int m = moduleByName (args[1]);
        if (m < 0) return 1;
        return screenshot (m, args[2] == "dark", file (args[3]), args.size() > 4 ? args[4] : juce::String());
    }

    if (args.size() > 1 && args[0] == "--levels")
    {
        // RMS change (dB) of every factory preset on the test phrase: the preset generator uses it to balance levels
        auto* root = new juce::DynamicObject();
        for (int m = 0; m < fx::kNumModules; ++m)
        {
            setCurrentModule (m);
            EffectProcessor p (m);
            auto* effect = new juce::DynamicObject();
            const auto& list = p.presetManager.getPresets();
            for (int i = 0; i < (int) list.size(); ++i)
            {
                p.presetManager.load (i);
                const auto r = render (p, 48000.0, 512, phrase (48000.0, 3.0));
                effect->setProperty (list[(size_t) i].name, juce::Decibels::gainToDecibels (r.rmsOut / std::max (1.0e-6f, r.rmsIn)));
            }
            root->setProperty (fx::info (m).id, juce::var (effect));
        }
        file (args[1]).replaceWithText (juce::JSON::toString (juce::var (root)));
        return 0;
    }

    if (args.size() > 0 && args[0] == "--strength")
    {
        const double sr = 48000.0;
        const auto dry = richPhrase (sr, 3.0);
        const float dryRms = std::max (dry.getRMSLevel (0, 0, dry.getNumSamples()), dry.getRMSLevel (1, 0, dry.getNumSamples()));
        auto* root = new juce::DynamicObject();
        for (int m = 0; m < fx::kNumModules; ++m)
        {
            setCurrentModule (m);
            EffectProcessor p (m);
            auto* effect = new juce::DynamicObject();
            std::vector<float> values;
            const auto& list = p.presetManager.getPresets();
            for (int i = 0; i < (int) list.size(); ++i)
            {
                p.presetManager.load (i);
                auto wet = richPhrase (sr, 3.0);
                renderInPlace (p, sr, 512, wet);
                float diff = 0.0f;
                for (int ch = 0; ch < 2; ++ch)
                {
                    wet.addFrom (ch, 0, dry, ch, 0, wet.getNumSamples(), -1.0f);
                    diff = std::max (diff, wet.getRMSLevel (ch, 0, wet.getNumSamples()));
                }
                const float db = juce::Decibels::gainToDecibels (diff / dryRms, -60.0f);
                effect->setProperty (list[(size_t) i].name, db);
                values.push_back (db);
            }
            std::sort (values.begin(), values.end());
            const int weak = (int) std::count_if (values.begin(), values.end(), [] (float v) { return v < -12.0f; });
            printf ("%-12s presets %3d   weakest %6.1f dB   median %6.1f dB   strongest %6.1f dB   below -12 dB: %d\n", fx::info (m).id,
                    (int) values.size(), values.front(), values[values.size() / 2], values.back(), weak);
            root->setProperty (fx::info (m).id, juce::var (effect));
        }
        if (args.size() > 1) file (args[1]).replaceWithText (juce::JSON::toString (juce::var (root)));
        return 0;
    }

    double sr = 48000.0;
    if (args.size() > 1 && args[0] == "--sr") sr = args[1].getDoubleValue();
    printf ("Fjord effects at %.0f Hz\n", sr);
    runTests (sr);
    printf (failures == 0 ? "ALL PASSED\n" : "%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
