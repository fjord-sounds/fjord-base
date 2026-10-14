// Offline test and tool binary of @Name@: renders sounds without a DAW and checks them.
//
//   @Name@RenderTest [out dir] [--sr 96000]                                  all tests (must print ALL PASSED)
//   @Name@RenderTest --screenshot <sound|fx|browser|about> <preset|init> <light|dark> <out.png> [fx index]
//   @Name@RenderTest --param-info <tsv>                                      id, kind, min, max, default per parameter
//   @Name@RenderTest --loudness <json>                                       RMS and peak of every factory preset
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static int failures = 0;
static double testSampleRate = 48000.0;

static void check (bool ok, const juce::String& what)
{
    if (! ok)
    {
        ++failures;
        printf ("FAIL  %s\n", what.toRawUTF8());
    }
}

struct Render
{
    float peak = 0.0f, rms = 0.0f, tail = 0.0f;
    bool finite = true;
};

// Plays one note for 'seconds' (released after 'hold') and measures the output.
static Render renderNote (@Name@Processor& proc, int note = 60, double hold = 0.5, double seconds = 3.0, int block = 512)
{
    proc.setPlayConfigDetails (0, 2, testSampleRate, block);
    proc.prepareToPlay (testSampleRate, block);
    juce::AudioBuffer<float> buffer (2, block);
    Render r;
    double sum = 0.0;
    int count = 0;
    const int total = (int) (seconds * testSampleRate), release = (int) (hold * testSampleRate);
    for (int pos = 0; pos < total; pos += block)
    {
        juce::MidiBuffer midi;
        if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (pos <= release && release < pos + block) midi.addEvent (juce::MidiMessage::noteOff (1, note), release - pos);
        buffer.clear();
        proc.processBlock (buffer, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
            {
                const float x = buffer.getSample (ch, i);
                r.finite &= std::isfinite (x);
                r.peak = std::max (r.peak, std::abs (x));
                if (pos + i < release + (int) (0.4 * testSampleRate)) { sum += (double) x * x; ++count; }
                if (pos + block >= total) r.tail = std::max (r.tail, std::abs (x));
            }
    }
    r.rms = (float) std::sqrt (sum / std::max (1, count));
    return r;
}

static int findPreset (@Name@Processor& proc, const juce::String& name)
{
    const auto& list = proc.presetManager.getPresets();
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].name == name) return (int) i;
    return -1;
}

static void soundTests()
{
    @Name@Processor proc;
    const auto r = renderNote (proc);
    printf ("init          peak %.3f  rms %.4f  tail %.1e\n", r.peak, r.rms, r.tail);
    check (r.finite, "init: finite output");
    check (r.peak > 0.01f && r.peak <= 1.0f, "init: level");
    check (r.tail < 1.0e-4f, "init: silent after the release");
}

static void blockSizeTest()
{
    for (int block : { 1, 37, 512, 4096 })
    {
        @Name@Processor proc;
        const auto r = renderNote (proc, 60, 0.3, 1.0, block);
        check (r.finite && r.peak > 0.01f, "block size " + juce::String (block));
    }
}

static void presetRoundTrip()
{
    @Name@Processor a, b;
    if (! a.presetManager.getPresets().empty()) a.presetManager.load (0);
    juce::MemoryBlock state;
    a.getStateInformation (state);
    b.setStateInformation (state.getData(), (int) state.getSize());
    for (const auto& s : @name@::params::specs())
        check (std::abs (a.apvts.getRawParameterValue (s.paramId)->load() - b.apvts.getRawParameterValue (s.paramId)->load()) < 1.0e-5f,
               juce::String ("state round trip: ") + s.paramId);
}

static void factoryPresetTests()
{
    @name@::PresetManager::ignoreUserPresets = true;
    @Name@Processor proc;
    const auto& list = proc.presetManager.getPresets();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        if (! list[(size_t) i].isFactory) continue;
        proc.presetManager.load (i);
        const auto r = renderNote (proc, 60, 0.5, 2.0);
        printf ("  %-12s %-22s peak %.3f  rms %.4f  %s\n", list[(size_t) i].category.toRawUTF8(), list[(size_t) i].name.toRawUTF8(),
                r.peak, r.rms, r.finite && r.peak <= 1.0f ? "OK" : "FAIL");
        check (r.finite && r.peak <= 1.0f && r.peak > 1.0e-3f, "preset " + list[(size_t) i].name);
    }
    printf ("  %d factory presets\n", (int) list.size());
}

static int paramInfo (const juce::File& out)
{
    juce::StringArray lines;
    for (const auto& s : @name@::params::specs())
    {
        const char* kinds[] = { "float", "int", "choice", "bool" };
        lines.add (juce::String (s.paramId) + "\t" + kinds[(int) s.kind] + "\t" + juce::String (s.min, 6) + "\t"
                   + juce::String (s.max, 6) + "\t" + juce::String (s.def, 6));
    }
    return out.replaceWithText (lines.joinIntoString ("\n") + "\n") ? 0 : 1;
}

static int loudness (const juce::File& out)
{
    @name@::PresetManager::ignoreUserPresets = true;
    @Name@Processor proc;
    auto* json = new juce::DynamicObject();
    const auto& list = proc.presetManager.getPresets();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        proc.presetManager.load (i);
        proc.apvts.getParameter ("preset_level")->setValueNotifyingHost (proc.apvts.getParameter ("preset_level")->convertTo0to1 (0.0f));
        const auto r = renderNote (proc);
        juce::Array<juce::var> v { juce::Decibels::gainToDecibels (r.rms, -100.0f), juce::Decibels::gainToDecibels (r.peak, -100.0f) };
        json->setProperty (list[(size_t) i].name, v);
    }
    return out.replaceWithText (juce::JSON::toString (juce::var (json))) ? 0 : 1;
}

static int takeScreenshot (const juce::String& pageName, const juce::String& presetName, const juce::String& theme,
                           const juce::File& outFile, int fxIndex)
{
    static const juce::StringArray pages { "sound", "fx", "browser", "about" };
    const int pageIndex = pages.indexOf (pageName);
    if (pageIndex < 0) { printf ("Unknown page: %s\n", pageName.toRawUTF8()); return 1; }

    @name@::PresetManager::ignoreUserPresets = true;
    fjord::base::fx::ChainPresets::ignoreUserPresets = true;
    @Name@Processor proc;
    if (presetName != "init")
    {
        const int index = findPreset (proc, presetName);
        if (index < 0) { printf ("Preset not found: %s\n", presetName.toRawUTF8()); return 1; }
        proc.presetManager.load (index);
    }
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    fjord::base::ui::Theme::get().setDark (theme == "dark");
    std::unique_ptr<juce::AudioProcessorEditor> editorWindow (proc.createEditor());
    editorWindow->setSize (@Name@View::width, @Name@View::height);
    auto* editor = dynamic_cast<@Name@Editor*> (editorWindow.get());
    if (editor == nullptr) return 1;
    auto& view = editor->getView();

    if (pageIndex == 2)      { view.selectPage (0); view.showBrowser (true); }
    else if (pageIndex == 3) { view.selectPage (0); view.showAbout(); }
    else                     view.selectPage (pageIndex);
    if (pageIndex == 1 && fxIndex >= 0)
        view.getFxPage().selectEffect (fxIndex);

    // Play a chord while the visuals animate, so they show the sound
    juce::AudioBuffer<float> buffer (2, 512);
    for (int k = 0; k < 60; ++k)
    {
        juce::MidiBuffer midi;
        if (k == 0) for (int note : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        buffer.clear();
        proc.processBlock (buffer, midi);
        view.tickAnimations();
    }

    const auto image = view.createComponentSnapshot (view.getLocalBounds(), true, 2.0f);
    outFile.getParentDirectory().createDirectory();
    outFile.deleteFile();
    if (auto stream = outFile.createOutputStream())
        juce::PNGImageFormat().writeImageToStream (image, *stream);
    editorWindow.reset();
    fjord::base::ui::Theme::get().setDark (false);
    return outFile.existsAsFile() ? 0 : 1;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    fjord::base::ui::Theme::get().setPersistent (false);   // tests never write user settings

    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (argv[i]);
    if (const int i = args.indexOf ("--sr"); i >= 0 && i + 1 < args.size())
    {
        testSampleRate = args[i + 1].getDoubleValue();
        args.removeRange (i, 2);
    }
    auto file = [] (const juce::String& path) { return juce::File::getCurrentWorkingDirectory().getChildFile (path); };

    if (args.size() > 1 && args[0] == "--param-info") return paramInfo (file (args[1]));
    if (args.size() > 1 && args[0] == "--loudness")   return loudness (file (args[1]));
    if (args.size() > 4 && args[0] == "--screenshot")
        return takeScreenshot (args[1], args[2], args[3], file (args[4]), args.size() > 5 ? args[5].getIntValue() : -1);

    printf ("@Name@ render test at %.0f Hz\n", testSampleRate);
    soundTests();
    blockSizeTest();
    presetRoundTrip();
    factoryPresetTests();

    printf (failures == 0 ? "ALL PASSED\n" : "%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
