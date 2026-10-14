#pragma once
#include <fjord_base/fx/Chain.h>
#include <fjord_base/fx/ModulePresets.h>
#include <fjord_base/presets/PresetManagerBase.h>
#include <fjord_base/plugin/PluginHelpers.h>

namespace fjord::effects
{
    namespace fx = fjord::base::fx;

    // The effect this binary is built for (FJORD_EFFECT_MODULE); the test tool switches it.
    int currentModule();
    void setCurrentModule (int module);

    // Presets of a single-effect plugin = the presets of that effect (factory + the user's, shared with every plugin).
    class EffectPresetManager : public fjord::base::presets::PresetManagerBase
    {
    public:
        EffectPresetManager (juce::AudioProcessorValueTreeState&, juce::UndoManager*, int module);

        bool saveUserPreset (const juce::String& name, const juce::String& category, const juce::StringArray& tags = {}) override;

    protected:
        void collectPresets (std::vector<Preset>&) const override;
        bool shouldLoadMacros() const override { return false; }
        bool shouldLoadEffects() const override { return false; }
        void applyExtras (const juce::XmlElement&, bool isInit) override;

    private:
        int module;
    };

    // One effect of the family as a plugin: the effect's parameters, an output level, presets, meters.
    class EffectProcessor : public juce::AudioProcessor
    {
    public:
        explicit EffectProcessor (int module);

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout&) const override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return JucePlugin_Name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override;

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock&) override;
        void setStateInformation (const void*, int) override;

        juce::String getDebugReport() const;
        void requestReset() { resetRequested.store (true); }

        const int module;
        juce::UndoManager undoManager;
        juce::AudioProcessorValueTreeState apvts;
        EffectPresetManager presetManager;
        fx::Meters meters;
        fjord::base::plugin::PeakMeter outputPeaks;

        static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (int module);

    private:
        fx::Chain chain;
        fjord::base::plugin::BlockSplitter splitter;
        std::array<std::atomic<float>*, fx::NumParams> raw {};
        std::array<float, fx::NumParams> values {};
        std::atomic<float>* outputGain = nullptr;
        juce::SmoothedValue<float> gain;
        std::atomic<bool> resetRequested { false };
        int triggerScratch = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectProcessor)
    };
}
