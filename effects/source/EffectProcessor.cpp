#include "EffectProcessor.h"
#include "EffectEditor.h"

namespace fjord::effects
{
    namespace
    {
        int module = FJORD_EFFECT_MODULE;

        // Accent per effect (light, dark): one hue per effect, all in the family's calm, northern range.
        struct Accent { juce::uint32 light, dark; };
        constexpr Accent accents[] = {
            { 0xffb0603a, 0xfff0a070 },   // saturation: ember
            { 0xff8a6a3a, 0xffd8b47a },   // lo-fi: faded tape
            { 0xffa8650a, 0xfff2b544 },   // transient: amber
            { 0xff2f68b0, 0xff6f9be8 },   // compressor: steel blue
            { 0xff7a5cc0, 0xffb49cf2 },   // squash: lilac
            { 0xff0e7566, 0xff58d1a4 },   // eq: aurora green
            { 0xffb0545a, 0xfff29c9c },   // filter: rose
            { 0xff1d7f93, 0xff3fb8c8 },   // motion: teal
            { 0xff2f7fb0, 0xff7fc4e8 },   // delay: ice blue
            { 0xff5a5fc0, 0xff9aa4f2 },   // reverb: night violet
            { 0xff1f9a7c, 0xff6fdcc0 },   // width: sea green
            { 0xffa0508a, 0xffe29ad0 },   // tremolo: heather
            { 0xff3a7a4a, 0xff8fd49a },   // shifter: moss
            { 0xffb08a1a, 0xfff2d060 },   // exciter: gold
            { 0xff8a4ab0, 0xffcf9cf2 },   // harmonizer: violet
            { 0xff5a6a7a, 0xffaebccb } }; // clipper: granite
    }

    int currentModule() { return module; }
    void setCurrentModule (int m) { module = juce::jlimit (0, fx::kNumModules - 1, m); }
}

const fjord::base::Brand& fjord::base::brand()
{
    using namespace fjord::effects;
    static std::array<std::unique_ptr<Brand>, fx::kNumModules> brands;
    auto& b = brands[(size_t) currentModule()];
    if (b == nullptr)
    {
        const int m = currentModule();
        const auto& info = fx::info (m);
        const auto accent = accents[(size_t) m];
        b = std::make_unique<Brand>();
        b->name = juce::String ("Fjord ") + info.name;
        b->logo = juce::String (info.name).toLowerCase();
        b->tagline = "FJORD EFFECT";
        b->githubUrl = "https://github.com/fjord-sounds/fjord-base";
        b->settingsFolder = "Fjord/Effects";
        b->userPresetFolder = "Fjord/FX Presets/" + juce::String (info.id);
        b->light = familyLight (juce::Colour (accent.light));
        b->dark = familyDark (juce::Colour (accent.dark));
        const auto l = juce::Colour (accent.light), d = juce::Colour (accent.dark);
        b->gradientLight = { l, l.withRotatedHue (0.08f), juce::Colour (0xff2f68b0) };
        b->gradientDark = { d, d.withRotatedHue (0.08f), juce::Colour (0xff6f9be8) };
        b->glow = { d, d.withRotatedHue (0.1f), juce::Colour (0xff5b8fe6) };
        b->flash = d.brighter (0.4f);
        for (const auto& p : fx::ModulePresets::get().getPresets (m))
            if (p.category != "User") b->categories.addIfNotAlreadyThere (p.category);
    }
    return *b;
}

namespace fjord::effects
{
    // ---------------------------------------------------------------------------------------------
    EffectPresetManager::EffectPresetManager (juce::AudioProcessorValueTreeState& s, juce::UndoManager* um, int m)
        : PresetManagerBase (s, um, { ".fjordmod", "FjordFxPreset", false, {}, { "master_gain" } }), module (m)
    {
    }

    void EffectPresetManager::collectPresets (std::vector<Preset>& out) const
    {
        const auto prefix = fx::ModulePresets::prefix (module);
        for (const auto& mp : fx::ModulePresets::get().getPresets (module))
        {
            juce::XmlElement xml ("FjordFxPreset");
            xml.setAttribute ("name", mp.name);
            xml.setAttribute ("category", mp.category);
            auto* on = xml.createNewChildElement ("Param");
            on->setAttribute ("id", prefix + "on");
            on->setAttribute ("value", 1.0);
            for (const auto& [key, value] : mp.values)
            {
                auto* e = xml.createNewChildElement ("Param");
                e->setAttribute ("id", prefix + key);
                e->setAttribute ("value", value);
            }
            Preset p;
            p.name = mp.name;
            p.category = mp.category;
            p.xml = xml.toString();
            p.file = mp.file;
            p.isFactory = mp.file == juce::File();
            out.push_back (std::move (p));
        }
    }

    // The effect's switch is the plugin's bypass: presets and Init leave it on.
    void EffectPresetManager::applyExtras (const juce::XmlElement&, bool)
    {
        if (auto* on = state.getParameter (fx::paramId (fx::onParam (module))))
            on->setValueNotifyingHost (1.0f);
    }

    bool EffectPresetManager::saveUserPreset (const juce::String& name, const juce::String&, const juce::StringArray&)
    {
        if (! fx::ModulePresets::get().saveUserPreset (state, module, name))
            return false;
        const auto clean = name.trim().isEmpty() ? juce::String ("My Preset") : name.trim();
        selectFile (fx::ModulePresets::getUserFolder (module).getChildFile (juce::File::createLegalFileName (clean) + ".fjordmod"), clean, "User");
        return true;
    }

    // ---------------------------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout EffectProcessor::createLayout (int m)
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        const auto& info = fx::info (m);
        for (int p = info.first; p <= info.last; ++p)
            layout.add (fjord::base::params::createParameter (fx::spec ((fx::Param) p)));
        layout.add (fjord::base::params::createParameter ({ 0, "master_gain", "Output", fjord::base::params::Kind::Float,
                                                            -24.0f, 12.0f, 0.0f, 0.0f, fjord::base::params::Format::Db, {} }));
        return layout;
    }

    EffectProcessor::EffectProcessor (int m)
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          module (m),
          apvts (*this, &undoManager, "FjordEffect", createLayout (m)),
          presetManager (apvts, &undoManager, m)
    {
        for (int p = 0; p < fx::NumParams; ++p)
            raw[(size_t) p] = apvts.getRawParameterValue (fx::paramId ((fx::Param) p));
        outputGain = apvts.getRawParameterValue ("master_gain");
        // The effect is on by default (its switch acts as bypass)
        if (auto* on = apvts.getParameter (fx::paramId (fx::onParam (m))))
            on->setValueNotifyingHost (1.0f);
        presetManager.initialise();
        undoManager.clearUndoHistory();
    }

    bool EffectProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto out = layouts.getMainOutputChannelSet(), in = layouts.getMainInputChannelSet();
        return out == juce::AudioChannelSet::stereo() && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
    }

    void EffectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        chain.prepare (sampleRate, samplesPerBlock);
        chain.reset();
        splitter.prepare (samplesPerBlock);
        gain.reset (sampleRate, 0.03);
        gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));
    }

    double EffectProcessor::getTailLengthSeconds() const
    {
        switch ((fx::Module) module)
        {
            case fx::Module::Reverb:     return 20.0;
            case fx::Module::Delay:      return 8.0;
            case fx::Module::Harmonizer: return 2.0;
            default:                     return 0.0;
        }
    }

    void EffectProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;
        if (getTotalNumInputChannels() == 1 && buffer.getNumChannels() > 1)
            buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

        if (resetRequested.exchange (false))
            chain.reset();

        const auto transport = fjord::base::plugin::Transport::read (*this);
        for (size_t p = 0; p < raw.size(); ++p)
            values[p] = raw[p] != nullptr ? raw[p]->load (std::memory_order_relaxed) : fx::defaults()[p];
        const fx::Values v { values.data() };
        const bool on = v.b (fx::onParam (module));
        gain.setTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));

        splitter.process (buffer, midi, [&] (juce::AudioBuffer<float>& part, juce::MidiBuffer&, int offset)
        {
            fx::Context ctx;
            ctx.bpm = transport.bpm;
            ctx.ppq = transport.ppq + transport.bpm / 60.0 * offset / getSampleRate();
            ctx.playing = transport.playing;
            ctx.triggers = nullptr;   // no notes: the filter envelope follows onsets in the audio
            fx::StereoBuffer b { part.getWritePointer (0), part.getWritePointer (1), part.getNumSamples() };
            if (on)
                chain.processSingle (module, b, v, ctx, &meters);
            else
                meters.out[(size_t) module].store (0.0f);
            for (int i = 0; i < part.getNumSamples(); ++i)
            {
                const float g = gain.getNextValue();
                b.l[i] *= g;
                b.r[i] *= g;
            }
            outputPeaks.push (part, 0, part.getNumSamples());
        });
    }

    void EffectProcessor::getStateInformation (juce::MemoryBlock& dest)
    {
        if (auto xml = apvts.copyState().createXml())
            copyXmlToBinary (*xml, dest);
    }

    void EffectProcessor::setStateInformation (const void* data, int size)
    {
        if (auto xml = getXmlFromBinary (data, size))
            if (xml->hasTagName (apvts.state.getType()))
            {
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
                presetManager.restoreFromState();
                undoManager.clearUndoHistory();
            }
    }

    juce::String EffectProcessor::getDebugReport() const
    {
        fjord::base::plugin::DebugReport report (*this, const_cast<EffectPresetManager*> (&presetManager), splitter.getMaxBlock());
        return report.finish();
    }

    juce::AudioProcessorEditor* EffectProcessor::createEditor() { return new EffectEditor (*this); }
}

#if ! FJORD_RENDER_TEST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new fjord::effects::EffectProcessor (FJORD_EFFECT_MODULE);
}
#endif
