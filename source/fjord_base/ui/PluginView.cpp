#include "PluginView.h"

namespace fjord::base::ui
{
    PluginView::PluginView (ViewSetup s, int w, int h)
        : baseWidth (w), baseHeight (h), setup (std::move (s)),
          presetBar (setup.presets),
          master (setup.state, setup.masterParamId, setup.masterLabel),
          browser (setup.presets),
          saveDialog (setup.presets),
          levelMeter (setup.peaks, setup.noteCounter),
          about (setup.debugReport, setup.panic)
    {
        setLookAndFeel (&lookAndFeel);
        Theme::get().addChangeListener (this);

        addAndMakeVisible (presetBar);
        addAndMakeVisible (master);
        addAndMakeVisible (levelMeter);
        addChildComponent (browser);
        addChildComponent (saveDialog);
        addChildComponent (about);

        presetBar.onOpenBrowser = [this] { setBrowserVisible (! browser.isVisible()); };
        presetBar.onSave = [this] { saveDialog.open(); };
        browser.onSave = [this] { saveDialog.open(); };
        browser.onClose = [this] { presetBar.setBrowserOpen (false); browserVisibilityChanged (false); };

        setWantsKeyboardFocus (false);
        applyTheme();
        startTimerHz (30);
    }

    PluginView::~PluginView()
    {
        Theme::get().removeChangeListener (this);
        setLookAndFeel (nullptr);
    }

    void PluginView::setPages (std::vector<juce::Component*> list)
    {
        pages = std::move (list);
        for (auto* p : pages)
            addChildComponent (p);
        // overlays stay on top of the pages
        browser.toFront (false);
        saveDialog.toFront (false);
        about.toFront (false);
        const int start = setup.rememberPage ? (int) setup.state.state.getProperty ("uiPage", 0) : 0;
        setSize (baseWidth, baseHeight);
        showPage (juce::jlimit (0, juce::jmax (0, (int) pages.size() - 1), start));
    }

    juce::Component* PluginView::currentPage() const
    {
        return juce::isPositiveAndBelow (page, (int) pages.size()) ? pages[(size_t) page] : nullptr;
    }

    void PluginView::changeListenerCallback (juce::ChangeBroadcaster*) { applyTheme(); }

    void PluginView::applyTheme()
    {
        lookAndFeel.applyPalette();
        sendLookAndFeelChange();
        repaint();
    }

    void PluginView::tickAnimations()
    {
        const bool offscreen = ! isTimerRunning() || ! isShowing();
        if (auto* pg = dynamic_cast<Page*> (currentPage()); pg != nullptr && pg->isVisible() && ! browser.isVisible())
            pg->tick();
        if (offscreen)
            levelMeter.tick();
        tickContent (offscreen);

        // The logo dot lights up with every note / hit
        if (setup.noteCounter != nullptr)
        {
            const auto notes = setup.noteCounter();
            if (notes != lastNotes)
            {
                lastNotes = notes;
                logoFlash = 0.4f + 0.6f * (setup.lastVelocity != nullptr ? juce::jlimit (0.0f, 1.0f, setup.lastVelocity()) : 0.8f);
                repaint (logoBounds());
            }
            else if (logoFlash > 0.0f)
            {
                logoFlash = std::max (0.0f, logoFlash - 0.08f);
                repaint (logoBounds());
            }
        }
    }

    void PluginView::setBrowserVisible (bool shouldShow)
    {
        if (shouldShow) browser.open();
        else            browser.close();
        presetBar.setBrowserOpen (shouldShow);
        browserVisibilityChanged (shouldShow);
    }

    void PluginView::showPage (int newPage)
    {
        setBrowserVisible (false);
        page = juce::jlimit (0, juce::jmax (0, (int) pages.size() - 1), newPage);
        for (size_t i = 0; i < pages.size(); ++i)
            pages[i]->setVisible ((int) i == page);
        if (setup.rememberPage)
            setup.state.state.setProperty ("uiPage", page, nullptr);
        pageChanged (page);
        repaint();
    }

    juce::Rectangle<int> PluginView::tabBounds (int index) const
    {
        return { margin + index * setup.tabWidth, headerHeight, setup.tabWidth, tabHeight };
    }

    juce::Rectangle<int> PluginView::themeToggleBounds() const
    {
        return { getWidth() - margin - 130, (headerHeight - 28) / 2, 28, 28 };
    }

    juce::Rectangle<int> PluginView::logoBounds() const
    {
        const float w = juce::GlyphArrangement::getStringWidth (logoFont (30.0f), brand().logo);
        return { margin, 0, (int) w + 20, headerHeight };
    }

    juce::Point<float> PluginView::logoDot() const
    {
        const float w = juce::GlyphArrangement::getStringWidth (logoFont (30.0f), brand().logo);
        return { (float) margin + 4.0f + w + 5.5f, (float) headerHeight * 0.5f + 7.5f };
    }

    juce::String PluginView::getTooltip()
    {
        switch (hover)
        {
            case Hover::Logo:   return "Click: about and debug info  |  Double-click: stop all sound";
            case Hover::Github: return brand().name + " on GitHub";
            case Hover::Theme:  return "Light / dark";
            case Hover::Undo:   return "Undo (Cmd+Z)";
            case Hover::Redo:   return "Redo (Cmd+Shift+Z)";
            case Hover::None:
            default:            return {};
        }
    }

    void PluginView::paint (juce::Graphics& g)
    {
        const auto& pal = Theme::get().palette();
        const bool dark = Theme::get().isDark();
        g.fillAll (pal.background);
        drawGlow (g, getLocalBounds().toFloat().withHeight ((float) (headerHeight + tabHeight) * 1.6f), dark);

        // Logo: lower case, light, with the accent dot that glows on every note
        g.setColour (pal.text);
        g.setFont (logoFont (30.0f));
        g.drawText (brand().logo, margin + 4, 0, logoBounds().getWidth() + 20, headerHeight, juce::Justification::centredLeft);
        const auto dot = logoDot();
        if (logoFlash > 0.0f)
        {
            const float rad = 6.0f + 16.0f * logoFlash;
            g.setGradientFill (juce::ColourGradient (pal.accent.withAlpha (0.55f * logoFlash), dot, pal.accent.withAlpha (0.0f),
                                                     dot.translated (rad, 0.0f), true));
            g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (dot));
        }
        g.setColour (pal.accent.interpolatedWith (dark ? juce::Colours::white : pal.accent, logoFlash * 0.5f));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (dot));

        // Tabs
        for (int i = 0; i < setup.tabs.size(); ++i)
        {
            const auto r = tabBounds (i).toFloat();
            const bool active = i == page;
            g.setColour (active ? pal.text : (i == hoveredTab ? pal.text.withAlpha (0.75f) : pal.textDim));
            drawSpacedText (g, setup.tabs[i].toUpperCase(), r, 11.0f, juce::Justification::centred, 0.2f, active);
            if (active)
            {
                g.setColour (pal.accent);
                g.fillRoundedRectangle (r.getCentreX() - 10.0f, r.getBottom() - 9.0f, 20.0f, 2.0f, 1.0f);
            }
        }

        // Undo / redo
        if (auto* um = setup.undo)
            for (bool redo : { false, true })
            {
                const auto r = (redo ? redoBounds() : undoBounds()).toFloat().reduced (5.0f);
                const bool can = redo ? um->canRedo() : um->canUndo();
                const bool hov = hover == (redo ? Hover::Redo : Hover::Undo);
                g.setColour (can ? (hov ? pal.text : pal.textDim) : pal.line);
                g.strokePath (icons::undoArrow (r, redo), juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

        icons::drawThemeSwitch (g, themeToggleBounds().toFloat().reduced (6.0f), dark, hover == Hover::Theme ? pal.text : pal.textDim);
        icons::drawGithub (g, githubBounds().toFloat().reduced (6.0f), hover == Hover::Github ? pal.text : pal.textDim);

        // Divider below the header
        g.setColour (pal.line);
        g.fillRect (margin, headerHeight - 1, getWidth() - 2 * margin, 1);
    }

    void PluginView::resized()
    {
        auto r = getLocalBounds();
        auto header = r.removeFromTop (headerHeight);
        master.setBounds (header.removeFromRight (margin + 92).withTrimmedRight (margin).reduced (0, 6));
        presetBar.setBounds (header.withSizeKeepingCentre (440, 36).withX ((getWidth() - 440) / 2));
        levelMeter.setBounds (logoBounds().getRight() + 14, (headerHeight - 30) / 2, setup.meterWidth, 30);

        // Plugins without tabs (single effects) use the tab row for content
        tabRowArea = r.removeFromTop (setup.tabs.isEmpty() ? 12 : tabHeight).withTrimmedLeft (margin + setup.tabWidth * setup.tabs.size()).withTrimmedRight (margin);
        layoutTabRow (tabRowArea);

        r.reduce (margin, 0);
        const auto browserArea = layoutContent (r);
        browser.setBounds (getLocalBounds().reduced (margin, 0).withTrimmedTop (headerHeight + 14).withBottom (browserArea.getBottom()));
        saveDialog.setBounds (getLocalBounds());
        about.setBounds (getLocalBounds());
    }

    void PluginView::mouseDown (const juce::MouseEvent& e)
    {
        for (int i = 0; i < setup.tabs.size(); ++i)
            if (tabBounds (i).contains (e.getPosition()))
                return showPage (i);

        if (themeToggleBounds().contains (e.getPosition()))
            Theme::get().setDark (! Theme::get().isDark());
        else if (githubBounds().contains (e.getPosition()))
            juce::URL (brand().githubUrl).launchInDefaultBrowser();
        else if (setup.undo != nullptr && undoBounds().contains (e.getPosition()))
            setup.undo->undo();
        else if (setup.undo != nullptr && redoBounds().contains (e.getPosition()))
            setup.undo->redo();
        repaint (0, 0, getWidth(), headerHeight);
    }

    void PluginView::mouseUp (const juce::MouseEvent& e)
    {
        if (! logoBounds().contains (e.getPosition()) || e.getNumberOfClicks() > 1 || e.mouseWasDraggedSinceMouseDown())
            return;
        const int click = ++logoClicks;
        juce::Timer::callAfterDelay (juce::MouseEvent::getDoubleClickTimeout() + 20,
                                     [safe = juce::Component::SafePointer<PluginView> (this), click]
                                     {
                                         if (safe != nullptr && safe->logoClicks == click)
                                             safe->about.open();
                                     });
    }

    void PluginView::mouseDoubleClick (const juce::MouseEvent& e)
    {
        if (logoBounds().contains (e.getPosition()))
        {
            ++logoClicks;   // cancels the pending About panel
            if (setup.panic) setup.panic();
        }
    }

    void PluginView::mouseMove (const juce::MouseEvent& e)
    {
        const auto pos = e.getPosition();
        int tab = -1;
        for (int i = 0; i < setup.tabs.size(); ++i)
            if (tabBounds (i).contains (pos))
                tab = i;

        auto h = Hover::None;
        if (logoBounds().contains (pos))                                   h = Hover::Logo;
        else if (themeToggleBounds().contains (pos))                       h = Hover::Theme;
        else if (githubBounds().contains (pos))                            h = Hover::Github;
        else if (setup.undo != nullptr && undoBounds().contains (pos))     h = Hover::Undo;
        else if (setup.undo != nullptr && redoBounds().contains (pos))     h = Hover::Redo;

        if (tab != hoveredTab || h != hover)
        {
            hoveredTab = tab;
            hover = h;
            setMouseCursor (tab >= 0 || h != Hover::None ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
            repaint (0, 0, getWidth(), headerHeight + tabHeight);
        }
    }

    void PluginView::mouseExit (const juce::MouseEvent&)
    {
        hoveredTab = -1;
        hover = Hover::None;
        repaint (0, 0, getWidth(), headerHeight + tabHeight);
    }

    bool PluginView::keyPressed (const juce::KeyPress& key)
    {
        if (setup.undo == nullptr) return false;
        if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0)) { setup.undo->undo(); repaint(); return true; }
        if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) { setup.undo->redo(); repaint(); return true; }
        return false;
    }
}
