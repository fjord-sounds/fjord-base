#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../presets/PresetManagerBase.h"
#include "../ui/Theme.h"

// Small pieces every processor needs: peak meters for the UI, splitting of oversized host blocks, the debug report.
namespace fjord::base::plugin
{
    // Output peaks for the level meter: the audio thread writes the maximum, the UI takes and resets it.
    class PeakMeter
    {
    public:
        void push (const juce::AudioBuffer<float>& buffer, int start, int num)
        {
            for (int ch = 0; ch < std::min (2, buffer.getNumChannels()); ++ch)
            {
                const float m = buffer.getMagnitude (ch, start, num);
                auto& slot = peaks[(size_t) ch];
                float prev = slot.load (std::memory_order_relaxed);
                while (m > prev && ! slot.compare_exchange_weak (prev, m, std::memory_order_relaxed)) {}
            }
            if (buffer.getNumChannels() == 1)
                peaks[1].store (peaks[0].load (std::memory_order_relaxed), std::memory_order_relaxed);
        }

        std::array<float, 2> take()
        {
            return { peaks[0].exchange (0.0f, std::memory_order_relaxed), peaks[1].exchange (0.0f, std::memory_order_relaxed) };
        }

    private:
        std::array<std::atomic<float>, 2> peaks {};
    };

    // Hosts may send blocks bigger than announced in prepareToPlay: this calls 'process' for chunks of at most
    // maxBlock samples, with the MIDI events of each chunk (no allocation after prepare).
    class BlockSplitter
    {
    public:
        void prepare (int maxBlockSize)
        {
            maxBlock = std::max (1, maxBlockSize);
            chunkMidi.ensureSize (8192);
        }

        int getMaxBlock() const { return maxBlock; }

        // process (buffer, midi, offsetInSamples)
        template <typename Fn>
        void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, Fn&& fn)
        {
            const int total = buffer.getNumSamples();
            if (total <= maxBlock)
            {
                fn (buffer, midi, 0);
                return;
            }
            for (int start = 0; start < total; start += maxBlock)
            {
                const int n = std::min (maxBlock, total - start);
                juce::AudioBuffer<float> part (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, n);
                chunkMidi.clear();
                for (const auto meta : midi)
                    if (meta.samplePosition >= start && meta.samplePosition < start + n)
                        chunkMidi.addEvent (meta.getMessage(), meta.samplePosition - start);
                fn (part, chunkMidi, start);
            }
            midi.clear();
        }

    private:
        int maxBlock = 512;
        juce::MidiBuffer chunkMidi;
    };

    // Host tempo and transport for one block (falls back to a given tempo when the host has none).
    struct Transport
    {
        double bpm = 120.0, ppq = 0.0;
        bool playing = false;

        static Transport read (juce::AudioProcessor& p, double fallbackBpm = 120.0)
        {
            Transport t;
            t.bpm = 0.0;
            if (auto* head = p.getPlayHead())
                if (auto pos = head->getPosition())
                {
                    t.playing = pos->getIsPlaying();
                    t.bpm = pos->getBpm().orFallback (0.0);
                    t.ppq = pos->getPpqPosition().orFallback (0.0);
                }
            if (! (t.bpm > 1.0 && t.bpm < 2000.0))
                t.bpm = fallbackBpm;
            return t;
        }
    };

    // The debug report behind "Copy debug info" in the About panel. The first block (up to the empty line) is shown
    // in the panel; the parameter list is only in the copied text. The home folder is written as ~.
    class DebugReport
    {
    public:
        DebugReport (const juce::AudioProcessor& p, presets::PresetManagerBase* presets, int preparedBlock)
        {
           #ifdef JucePlugin_VersionString
            juce::String version = JucePlugin_VersionString;
           #else
            juce::String version = "dev";
           #endif
           #if JUCE_ARM
            const juce::String arch = "arm64";
           #elif JUCE_64BIT
            const juce::String arch = "x86_64";
           #else
            const juce::String arch = "32-bit";
           #endif
           #ifdef FJORD_GIT_HASH
            const juce::String build = juce::String (FJORD_GIT_HASH) + ", ";
           #else
            const juce::String build;
           #endif
           #ifdef FJORD_BASE_GIT_HASH
            const juce::String baseBuild = juce::String (" (") + FJORD_BASE_GIT_HASH + ")";
           #else
            const juce::String baseBuild;
           #endif

            const auto title = brand().name + " debug info";
            lines.add (title);
            lines.add (juce::String::repeatedString ("-", title.length()));
            add ("Version", version + " (" + build + arch + ", built " + __DATE__ + ")");
            add ("fjord-base", juce::String (FJORD_BASE_VERSION) + baseBuild);
            add ("Format", juce::AudioProcessor::getWrapperTypeDescription (p.wrapperType));
            add ("Host", juce::PluginHostType().getHostDescription());
            add ("Plugin", juce::File::getSpecialLocation (juce::File::currentApplicationFile).getFullPathName());
            if (presets != nullptr)
                add ("User presets", presets::PresetManagerBase::getUserPresetFolder().getFullPathName());
            add ("Settings", ui::Theme::get().getSettingsFile().getFullPathName());
            add ("Sample rate", juce::String (p.getSampleRate(), 0) + " Hz, block " + juce::String (p.getBlockSize())
                                + " (prepared " + juce::String (preparedBlock) + ")");
            add ("OS", juce::SystemStats::getOperatingSystemName() + (juce::SystemStats::isOperatingSystem64Bit() ? " 64-bit" : ""));
            add ("CPU", juce::SystemStats::getCpuModel().trim() + ", " + juce::String (juce::SystemStats::getNumCpus()) + " cores");
            add ("JUCE", juce::SystemStats::getJUCEVersion());
            add ("Theme / scale", juce::String (ui::Theme::get().isDark() ? "dark" : "light") + ", "
                                  + juce::String (juce::roundToInt (ui::Theme::get().getScale() * 100.0f)) + " %");
            if (presets != nullptr)
            {
                int factory = 0, user = 0;
                for (const auto& pr : presets->getPresets())
                    (pr.isFactory ? factory : user)++;
                add ("Presets", juce::String (factory) + " factory, " + juce::String (user) + " user");
                add ("Preset", presets->getCurrentName() + " (" + presets->getCurrentCategory() + ")");
            }
            processor = &p;
        }

        // A key / value line in the panel block (call before finish)
        void add (const juce::String& key, const juce::String& value)
        {
            const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName();
            lines.add (key.paddedRight (' ', 16) + (home.length() > 1 ? value.replace (home, "~") : value));
        }

        // Adds the changed parameters (and anything else the plugin appends afterwards with 'more').
        juce::String finish (const juce::StringArray& more = {})
        {
            juce::StringArray changed;
            for (auto* prm : processor->getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                    if (std::abs (rp->getValue() - rp->getDefaultValue()) > 1.0e-5f)
                        changed.add (rp->getParameterID() + "=" + juce::String (rp->convertFrom0to1 (rp->getValue()), 4));
            lines.add ("");
            lines.add ("Changed parameters (" + juce::String (changed.size()) + "):");
            lines.add (changed.joinIntoString (" "));
            lines.addArray (more);
            return lines.joinIntoString ("\n");
        }

    private:
        const juce::AudioProcessor* processor = nullptr;
        juce::StringArray lines;
    };
}
