#pragma once
#include "Theme.h"
#include "../Parameters.h"
#include "../presets/Locks.h"

namespace @name@::ui
{
    // The first page: oscillator, filter and envelope. A page is a fjord::base::ui::Page; tick() runs with the
    // UI timer (animations).
    class SoundPage : public Page
    {
    public:
        explicit SoundPage (juce::AudioProcessorValueTreeState& state)
            : wave (state, params::paramId (params::Wave)),
              cutoff (state, params::paramId (params::Cutoff), "Cutoff"),
              reso (state, params::paramId (params::Resonance), "Resonance"),
              attack (state, params::paramId (params::Attack), "Attack"),
              release (state, params::paramId (params::Release), "Release"),
              oscLock (state, locks::set(), locks::ids().osc, false),
              filterLock (state, locks::set(), locks::ids().filter, false),
              ampLock (state, locks::set(), locks::ids().amp, false)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &oscSection, &filterSection, &ampSection, &wave,
                                                                     &cutoff, &reso, &attack, &release, &oscLock, &filterLock, &ampLock })
                addAndMakeVisible (c);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            const int w = (r.getWidth() - 24) / 3;
            oscSection.setBounds (r.removeFromLeft (w));
            r.removeFromLeft (12);
            filterSection.setBounds (r.removeFromLeft (w));
            r.removeFromLeft (12);
            ampSection.setBounds (r);

            oscSection.placeTitleLock (oscLock);   // the section's title row is in its own coordinates
            oscLock.setBounds (oscLock.getBounds().translated (oscSection.getX(), oscSection.getY()));
            filterSection.placeTitleLock (filterLock);
            filterLock.setBounds (filterLock.getBounds().translated (filterSection.getX(), filterSection.getY()));
            ampSection.placeTitleLock (ampLock);
            ampLock.setBounds (ampLock.getBounds().translated (ampSection.getX(), ampSection.getY()));

            // Section::content() is relative to the section: move it into the page's coordinates
            auto content = [] (const Section& section) { return section.content().translated (section.getX(), section.getY()); };
            auto osc = content (oscSection);
            wave.setBounds (osc.removeFromTop (60).withSizeKeepingCentre (osc.getWidth() - 24, 28));
            layoutRow (content (filterSection).removeFromTop (110), { &cutoff, &reso });
            layoutRow (content (ampSection).removeFromTop (110), { &attack, &release });
        }

    private:
        Section oscSection { "Oscillator" }, filterSection { "Filter" }, ampSection { "Amp" };
        Segmented wave;
        Knob cutoff, reso, attack, release;
        LockButton oscLock, filterLock, ampLock;   // keep this part when switching presets
    };
}
