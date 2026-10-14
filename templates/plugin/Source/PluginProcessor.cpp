#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace P = @name@::params;

// Name, logo, colours, categories and tags of @Name@: everything the shared UI of fjord-base draws comes from here.
// Every colour can be set per plugin (see fjord-base/docs/design-system.md); familyLight / familyDark derive the
// family palette from one accent.
const fjord::base::Brand& fjord::base::brand()
{
    static const Brand b = []
    {
        Brand x;
        x.name = "@Name@";
        x.logo = "@name@";
        x.tagline = "@TAGLINE@";
        x.githubUrl = "https://github.com/fjord-sounds/@name@";
        x.settingsFolder = "Fjord/@Name@";
        x.userPresetFolder = "Fjord/@Name@/Presets";
        x.light = familyLight (juce::Colour (@ACCENT_LIGHT@));
        x.dark = familyDark (juce::Colour (@ACCENT_DARK@));
        x.gradientLight = { juce::Colour (@ACCENT_LIGHT@), juce::Colour (0xff1d7f93), juce::Colour (0xff2f68b0) };
        x.gradientDark = { juce::Colour (@ACCENT_DARK@), juce::Colour (0xff3fb8c8), juce::Colour (0xff6f9be8) };
        x.glow = { juce::Colour (@ACCENT_DARK@), juce::Colour (0xff3fb8c8), juce::Colour (0xff5b8fe6) };
        x.flash = juce::Colour (@ACCENT_DARK@).brighter (0.4f);
        x.categories = { "Basics" };
        x.tags = { "Warm", "Bright", "Soft" };
        return x;
    }();
    return b;
}

namespace
{
    // Version of the saved project state (not the plugin version). Raise it when a change needs the old state
    // to be converted (fjord-base/source/fjord_base/presets/Migration.h helps), and convert it in setStateInformation.
    constexpr int kStateVersion = 1;
}

@Name@Processor::@Name@Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "@Name@State", P::createLayout())
{
    undoManager.clearUndoHistory();
}

void @Name@Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = std::max (16, samplesPerBlock);
    synth.prepare (sampleRate);
    fxChain.prepare (sampleRate, maxBlockSize);
    splitter.prepare (maxBlockSize);
    masterGain.reset (sampleRate, 0.02);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (P::paramId (P::MasterGain))->load()));
    presetLevel.reset (sampleRate, 0.02);
    presetLevel.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (P::paramId (P::PresetLevel))->load()));
}

bool @Name@Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void @Name@Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    if (panicRequested.exchange (false))
    {
        midi.clear();
        synth.reset();
        fxChain.reset();
    }

    // Hosts may send bigger blocks than announced: the splitter keeps every chunk within maxBlockSize.
    const auto transport = fjord::base::plugin::Transport::read (*this);
    splitter.process (buffer, midi, [&] (juce::AudioBuffer<float>& part, juce::MidiBuffer& partMidi, int offset)
    {
        processChunk (part, partMidi, transport.ppq + transport.bpm / 60.0 * offset / currentSampleRate, transport.playing, transport.bpm);
    });
}

void @Name@Processor::processChunk (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, double ppq, bool playing, double bpm)
{
    const int n = buffer.getNumSamples();
    reader.read (values);
    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);

    // Notes are rendered sample-accurately: render up to each event, then apply it
    int pos = 0, numNotes = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int at = juce::jlimit (0, n, meta.samplePosition);
        synth.render (l + pos, r + pos, at - pos, values);
        pos = at;
        if (m.isNoteOn())
        {
            synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
            lastVelocity.store (m.getFloatVelocity());
            noteCount.fetch_add (1);
            if (numNotes < (int) notePositions.size()) notePositions[(size_t) numNotes++] = at;
        }
        else if (m.isNoteOff())                         synth.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allNotesOff();
    }
    synth.render (l + pos, r + pos, n - pos, values);

    presetLevel.setTargetValue (juce::Decibels::decibelsToGain (values[P::PresetLevel]));
    presetLevel.applyGain (buffer, n);

    // The shared effects, in the user's order; the clipper (Out) always comes last
    fjord::base::fx::Context ctx;
    ctx.bpm = bpm;
    ctx.ppq = ppq;
    ctx.playing = playing;
    ctx.triggers = notePositions.data();   // the filter envelope starts with each note
    ctx.numTriggers = numNotes;
    fxChain.process ({ l, r, n }, P::fxValues (values), fxOrder.get(), ctx, &fxMeters);

    masterGain.setTargetValue (juce::Decibels::decibelsToGain (values[P::MasterGain]));
    masterGain.applyGain (buffer, n);
    outputPeaks.push (buffer, 0, n);
}

int @Name@Processor::getNumPrograms()
{
    int n = 0;
    for (const auto& p : presetManager.getPresets())
        n += p.isFactory ? 1 : 0;
    return std::max (1, n);
}

int @Name@Processor::getCurrentProgram() { return std::max (0, presetManager.getCurrentIndex()); }

void @Name@Processor::setCurrentProgram (int index)
{
    if (index != presetManager.getCurrentIndex() && index < getNumPrograms())
        presetManager.load (index);
}

const juce::String @Name@Processor::getProgramName (int index)
{
    const auto& list = presetManager.getPresets();
    return juce::isPositiveAndBelow (index, (int) list.size()) ? list[(size_t) index].name : juce::String ("Init");
}

juce::String @Name@Processor::getDebugReport() const
{
    fjord::base::plugin::DebugReport report (*this, const_cast<@name@::PresetManager*> (&presetManager), maxBlockSize);
    report.add ("FX order", apvts.state.getProperty ("fxOrder").toString());
    return report.finish();
}

juce::AudioProcessorEditor* @Name@Processor::createEditor() { return new @Name@Editor (*this); }

void @Name@Processor::getStateInformation (juce::MemoryBlock& dest)
{
    apvts.state.setProperty ("stateVersion", kStateVersion, nullptr);
    apvts.state.setProperty ("darkTheme", fjord::base::ui::Theme::get().isDark(), nullptr);
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void @Name@Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            presetManager.restoreFromState();
            fxOrder.sync();
            undoManager.clearUndoHistory();

            if (apvts.state.hasProperty ("darkTheme"))
            {
                const bool dark = apvts.state.getProperty ("darkTheme");
                juce::MessageManager::callAsync ([dark] { fjord::base::ui::Theme::get().setDark (dark); });
            }
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new @Name@Processor();
}
