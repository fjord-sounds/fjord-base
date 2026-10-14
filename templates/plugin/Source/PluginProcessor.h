#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "dsp/Synth.h"
#include "presets/PresetManager.h"
#include <fjord_base/fx/Chain.h>
#include <fjord_base/fx/OrderState.h>
#include <fjord_base/fx/ChainPresets.h>
#include <fjord_base/plugin/PluginHelpers.h>

class @Name@Processor : public juce::AudioProcessor
{
public:
    @Name@Processor();
    ~@Name@Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }   // reverb and delay tails

    // Factory presets also show up in the host's preset / program list.
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;                 // on-screen keyboard
    @name@::PresetManager presetManager { apvts, &undoManager };
    fjord::base::fx::ChainPresets chainPresets { apvts };   // effect chains (shared by every Fjord plugin)
    fjord::base::fx::Meters fxMeters;
    fjord::base::fx::OrderState fxOrder { apvts, &undoManager };

    // Visual feedback: the logo dot flashes with every note
    uint32_t getNoteCount() const { return noteCount.load(); }
    float getLastVelocity() const { return lastVelocity.load(); }
    std::array<float, 2> takeOutputPeaks() { return outputPeaks.take(); }

    void requestPanic() { panicRequested.store (true); }
    juce::String getDebugReport() const;

private:
    void processChunk (juce::AudioBuffer<float>&, juce::MidiBuffer&, double ppq, bool playing, double bpm);

    @name@::params::Reader reader { apvts };
    @name@::params::Values values;
    @name@::dsp::Synth synth;
    fjord::base::fx::Chain fxChain;
    fjord::base::plugin::BlockSplitter splitter;
    fjord::base::plugin::PeakMeter outputPeaks;
    juce::SmoothedValue<float> masterGain, presetLevel;
    std::array<int, 128> notePositions {};
    std::atomic<uint32_t> noteCount { 0 };
    std::atomic<float> lastVelocity { 0.8f };
    std::atomic<bool> panicRequested { false };
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (@Name@Processor)
};
