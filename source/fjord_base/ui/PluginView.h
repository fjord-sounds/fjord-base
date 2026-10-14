#pragma once
#include "LookAndFeel.h"
#include "Header.h"
#include "PresetBar.h"
#include "PresetBrowser.h"
#include "Icons.h"

namespace fjord::base::ui
{
    // Base of all pages: tick() is called at 30 Hz while the page is visible (and by hand for screenshots).
    class Page : public juce::Component
    {
    public:
        virtual void tick() {}
    };

    // Everything the shared view needs from the plugin.
    struct ViewSetup
    {
        juce::AudioProcessorValueTreeState& state;
        PresetManagerBase& presets;
        juce::UndoManager* undo = nullptr;                    // nullptr: no undo / redo buttons
        juce::String masterParamId { "master_gain" };
        juce::String masterLabel { "Master" };
        juce::StringArray tabs;
        int tabWidth = 84;
        std::function<std::array<float, 2>()> peaks;           // output level for the meter
        std::function<uint32_t()> noteCounter;                 // counts notes / hits: the logo dot and the meter flash
        std::function<float()> lastVelocity;                   // 0..1, how bright the logo dot flashes
        std::function<juce::String()> debugReport;             // About panel + "copy debug info"
        std::function<void()> panic;                           // double-click on the logo, "Stop notes"
        bool rememberPage = true;                              // the open page is stored in the state ("uiPage")
        int meterWidth = 170;
    };

    // The common frame of every plugin window, drawn at a fixed size and scaled as a whole by ScaledEditor:
    //
    //   logo. ▁▂▃▅ meter      ‹  CATEGORY · Preset name  ›  ··· ⤓        ↶ ↷  github  ☀  (master)
    //   TAB  TAB  TAB  TAB                                          (plugin extras in the tab row)
    //   ──────────────────────────────────────────────────────────────────────────────────────────
    //   pages ...                                                        (plugin content)
    //   footer / keyboard ...
    //
    // The logo dot flashes with every note, a click on the logo opens the About panel, a double-click stops all sound.
    class PluginView : public juce::Component,
                       public juce::TooltipClient,
                       private juce::ChangeListener,
                       private juce::Timer
    {
    public:
        static constexpr int headerHeight = 64, tabHeight = 40, margin = 16;

        PluginView (ViewSetup, int baseWidth, int baseHeight);
        ~PluginView() override;

        const int baseWidth, baseHeight;

        // Pages in tab order (they are added as children here).
        void setPages (std::vector<juce::Component*>);
        void showPage (int index);
        int getPage() const { return page; }
        juce::Component* currentPage() const;

        void showBrowser (bool shouldShow) { setBrowserVisible (shouldShow); }
        bool isBrowserOpen() const { return browser.isVisible(); }
        void showAbout() { about.open(); }
        void showSaveDialog() { saveDialog.open(); }

        // One animation frame; the screenshot tool calls it by hand (the timer does not run offscreen).
        void tickAnimations();

        PresetBar& getPresetBar() { return presetBar; }
        LevelMeter& getLevelMeter() { return levelMeter; }

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;
        juce::String getTooltip() override;

    protected:
        // Lays out the plugin's content below the tabs (pages, footer, keyboard ...). Returns the area the preset
        // browser may cover (usually the page area).
        virtual juce::Rectangle<int> layoutContent (juce::Rectangle<int> area) = 0;
        // Extras in the tab row (right of the tabs), e.g. mode chips.
        virtual void layoutTabRow (juce::Rectangle<int>) {}
        virtual void browserVisibilityChanged (bool) {}
        // Called at 30 Hz (also offscreen for screenshots): footers, visuals ...
        virtual void tickContent (bool offscreen) { juce::ignoreUnused (offscreen); }
        virtual void pageChanged (int) {}

        juce::Rectangle<int> tabRow() const { return tabRowArea; }

        ViewSetup setup;
        LookAndFeel lookAndFeel;

    private:
        enum class Hover { None, Logo, Theme, Github, Undo, Redo };

        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        void timerCallback() override { tickAnimations(); }
        void applyTheme();
        void setBrowserVisible (bool);

        juce::Rectangle<int> tabBounds (int index) const;
        juce::Rectangle<int> themeToggleBounds() const;
        juce::Rectangle<int> githubBounds() const { return themeToggleBounds().translated (-36, 0); }
        juce::Rectangle<int> redoBounds() const { return githubBounds().translated (-44, 0); }
        juce::Rectangle<int> undoBounds() const { return redoBounds().translated (-30, 0); }
        juce::Rectangle<int> logoBounds() const;
        juce::Point<float> logoDot() const;

        juce::TooltipWindow tooltips { this, 600 };
        PresetBar presetBar;
        Knob master;
        PresetBrowser browser;
        SaveDialog saveDialog;
        LevelMeter levelMeter;
        AboutPanel about;

        std::vector<juce::Component*> pages;
        juce::Rectangle<int> tabRowArea;
        int page = 0, hoveredTab = -1, logoClicks = 0;
        Hover hover = Hover::None;
        uint32_t lastNotes = 0;
        float logoFlash = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginView)
    };

    // Plugin window: freely scalable (75 - 150 %) with a fixed aspect ratio. The size is remembered per project
    // ("uiScale" in the state) and globally (the last size used).
    template <typename ViewType, typename ProcessorType>
    class ScaledEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit ScaledEditor (ProcessorType& p) : juce::AudioProcessorEditor (p), proc (p), view (p)
        {
            addAndMakeVisible (view);
            setResizable (true, true);
            setResizeLimits (view.baseWidth * 3 / 4, view.baseHeight * 3 / 4, view.baseWidth * 3 / 2, view.baseHeight * 3 / 2);
            if (auto* c = getConstrainer())
                c->setFixedAspectRatio ((double) view.baseWidth / view.baseHeight);

            const auto& st = p.apvts.state;
            const float saved = st.hasProperty ("uiScale") ? (float) (double) st.getProperty ("uiScale") : Theme::get().getScale();
            const float scale = juce::jlimit (0.75f, 1.5f, saved);
            setSize (juce::roundToInt ((float) view.baseWidth * scale), juce::roundToInt ((float) view.baseHeight * scale));
            ready = true;
        }

        void resized() override
        {
            const float scale = (float) getWidth() / (float) view.baseWidth;
            view.setTransform (juce::AffineTransform::scale (scale));
            view.setBounds (0, 0, view.baseWidth, view.baseHeight);

            if (ready && scale >= 0.74f && scale <= 1.51f)
            {
                Theme::get().setScale (scale);
                proc.apvts.state.setProperty ("uiScale", scale, nullptr);
            }
        }

        ViewType& getView() { return view; }

    protected:
        ProcessorType& proc;
        ViewType view;
        bool ready = false;
    };
}
