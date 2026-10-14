#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FxParams.h"

namespace fjord::base::fx
{
    // The effect order of a plugin: stored as "fxOrder" in the state (and in presets), mirrored into atomics so the
    // audio thread reads it without locks. Old formats are converted by the plugin's legacy decoder.
    class OrderState : private juce::ValueTree::Listener
    {
    public:
        // legacy: converts an old order string (e.g. effect indices) to the id format; may be nullptr
        OrderState (juce::AudioProcessorValueTreeState& s, juce::UndoManager* um, Order fallbackOrder = defaultOrder(),
                    std::function<juce::String (const juce::String&)> legacy = nullptr)
            : state (s), undo (um), fallback (fallbackOrder), legacyDecoder (std::move (legacy))
        {
            if (! state.state.hasProperty (prop))
                state.state.setProperty (prop, orderToString (fallback), nullptr);
            state.state.addListener (this);
            sync();
        }

        ~OrderState() override { state.state.removeListener (this); }

        Order get() const
        {
            Order o;
            for (size_t i = 0; i < o.size(); ++i) o[i] = atoms[i].load (std::memory_order_relaxed);
            return o;
        }

        void set (const Order& o) { state.state.setProperty (prop, orderToString (o), undo); }

        // Any stored text (old or new format) -> the current id format
        juce::String normalise (const juce::String& text) const
        {
            const auto t = legacyDecoder != nullptr ? legacyDecoder (text) : text;
            return orderToString (orderFromString (t, fallback));
        }

        // Call after replacing the state (a project was loaded)
        void sync()
        {
            const auto stored = state.state.getProperty (prop).toString();
            const auto clean = normalise (stored);
            if (clean != stored)
                state.state.setProperty (prop, clean, nullptr);
            const auto o = orderFromString (clean, fallback);
            for (size_t i = 0; i < o.size(); ++i) atoms[i].store (o[i]);
        }

        const Order& getFallback() const { return fallback; }

    private:
        void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& id) override
        {
            if (tree == state.state && id == prop && ! syncing)
            {
                const juce::ScopedValueSetter<bool> guard (syncing, true);
                sync();
            }
        }

        static inline const juce::Identifier prop { "fxOrder" };
        juce::AudioProcessorValueTreeState& state;
        juce::UndoManager* undo;
        Order fallback;
        std::function<juce::String (const juce::String&)> legacyDecoder;
        std::array<std::atomic<int>, kNumChain> atoms {};
        bool syncing = false;
    };
}
