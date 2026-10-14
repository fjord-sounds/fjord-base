#pragma once
#include "Graphs.h"
#include "ModulePresets.h"
#include "ChainPresets.h"
#include "../ui/PluginView.h"

namespace fjord::base::fx
{
    using ui::APVTS;
    using ui::Knob;
    using ui::Segmented;
    using ui::Toggle;
    using ui::Choice;
    using ui::ControlRow;
    using ui::drawSpacedText;
    using ui::font;
    using ui::Weight;
    using ui::PillButton;
    using ui::styleTextEditor;

    // What the effect page needs from the plugin.
    struct Host
    {
        APVTS& state;
        juce::UndoManager* undo = nullptr;
        const Meters& meters;
        std::function<Order()> getOrder;                    // nullptr: a single effect, no chain
        std::function<void (const Order&)> setOrder;
        ChainPresets* chains = nullptr;                     // effect chain presets + "Lock FX" bar on top (optional)
    };

    // Reads the current plain values of the whole effect block from the parameters (UI thread).
    class UiValues
    {
    public:
        explicit UiValues (APVTS& s)
        {
            for (int i = 0; i < NumParams; ++i)
                raw[(size_t) i] = s.getRawParameterValue (paramId ((Param) i));
            update();
        }

        void update()
        {
            for (size_t i = 0; i < raw.size(); ++i)
                values[i] = raw[i] != nullptr ? raw[i]->load() : defaults()[i];
        }

        Values view() const { return { values.data() }; }

    private:
        std::array<std::atomic<float>*, NumParams> raw {};
        std::array<float, NumParams> values {};
    };

    // Controls of every effect module (in the order they appear in the editor). Units: knobs 1, selectors wider.
    inline void buildControls (int module, APVTS& s, ControlRow& row)
    {
        auto knob = [&] (Param p, const char* label) { row.add (std::make_unique<Knob> (s, paramId (p), label)); };
        auto seg = [&] (Param p, const char* label, float units, juce::StringArray names = {})
        {
            row.add (ui::labelled (std::make_unique<Segmented> (s, paramId (p), nullptr, names), label), units);
        };
        auto choice = [&] (Param p, const char* label, float units = 2.0f)
        {
            row.add (ui::labelled (std::make_unique<Choice> (s, paramId (p)), label), units);
        };
        auto toggle = [&] (Param p, const char* label, const char* title)
        {
            row.add (ui::labelled (std::make_unique<Toggle> (s, paramId (p), label), title), 1.2f);
        };

        switch ((Module) module)
        {
            case Module::Saturation: seg (FxSatMode, "Curve", 4.5f); knob (FxSatDrive, "Drive"); knob (FxSatBias, "Bias"); knob (FxSatTone, "Tone");
                                     seg (FxSatOs, "Oversampling", 2.0f); knob (FxSatOut, "Output"); toggle (FxSatAuto, "Auto", "Gain"); knob (FxSatMix, "Mix"); break;
            case Module::Lofi:       knob (FxLofiWow, "Wow"); knob (FxLofiFlutter, "Flutter"); knob (FxLofiAge, "Age"); knob (FxLofiNoise, "Hiss");
                                     knob (FxLofiBits, "Bits"); knob (FxLofiRate, "Downsample"); knob (FxLofiJitter, "Jitter"); knob (FxLofiMix, "Mix"); break;
            case Module::Transient:  knob (FxTransAttack, "Attack"); knob (FxTransSustain, "Sustain"); knob (FxTransSpeed, "Speed"); knob (FxTransOut, "Output"); knob (FxTransMix, "Mix"); break;
            case Module::Compressor: knob (FxCompThresh, "Threshold"); knob (FxCompRatio, "Ratio"); knob (FxCompAttack, "Attack"); knob (FxCompRelease, "Release");
                                     knob (FxCompKnee, "Knee"); knob (FxCompMakeup, "Makeup"); toggle (FxCompAuto, "Auto", "Gain"); knob (FxCompHpf, "SC Filter"); knob (FxCompMix, "Mix"); break;
            case Module::Squash:     knob (FxSqDepth, "Depth"); knob (FxSqTime, "Time"); knob (FxSqUpward, "Upward"); knob (FxSqOut, "Output"); knob (FxSqMix, "Mix"); break;
            case Module::Eq:         knob (FxEqLowCut, "Low Cut"); seg (FxEqLowSlope, "Slope", 1.2f, { "12", "24" }); knob (FxEqLowFreq, "Low Freq"); knob (FxEqLowGain, "Low");
                                     knob (FxEqMidFreq, "Mid Freq"); knob (FxEqMidGain, "Mid"); knob (FxEqMidQ, "Mid Q");
                                     knob (FxEqMid2Freq, "Mid 2 Freq"); knob (FxEqMid2Gain, "Mid 2"); knob (FxEqMid2Q, "Mid 2 Q");
                                     knob (FxEqHighFreq, "High Freq"); knob (FxEqHighGain, "High"); knob (FxEqHighCut, "High Cut"); seg (FxEqHighSlope, "Slope", 1.2f, { "12", "24" }); break;
            case Module::Filter:     seg (FxFiltType, "Type", 4.0f, { "Ladder", "LP", "BP", "HP", "Notch" }); knob (FxFiltCutoff, "Cutoff"); knob (FxFiltReso, "Resonance");
                                     knob (FxFiltDrive, "Drive"); knob (FxFiltEnv, "Env Amount"); knob (FxFiltDecay, "Env Decay"); seg (FxFiltTrigger, "Trigger", 2.0f);
                                     choice (FxFiltLfoShape, "LFO Shape"); toggle (FxFiltLfoSync, "Sync", "Tempo"); knob (FxFiltLfoRate, "LFO Rate");
                                     choice (FxFiltLfoNote, "LFO Note"); knob (FxFiltLfoDepth, "LFO Depth"); knob (FxFiltMix, "Mix"); break;
            case Module::Motion:     seg (FxMotMode, "Mode", 4.5f, { "Chorus", "Ens", "Phaser", "Flanger", "Double", "Vibrato" }); toggle (FxMotSync, "Sync", "Tempo");
                                     knob (FxMotRate, "Rate"); choice (FxMotNote, "Note"); knob (FxMotDepth, "Depth"); knob (FxMotFeedback, "Feedback"); knob (FxMotMix, "Mix"); break;
            case Module::Delay:      seg (FxDlyMode, "Character", 3.5f); toggle (FxDlySync, "Sync", "Tempo"); choice (FxDlyNote, "Note"); knob (FxDlyTime, "Time");
                                     knob (FxDlyFeedback, "Feedback"); toggle (FxDlyPingPong, "Ping-Pong", "Stereo"); knob (FxDlySpread, "Offset"); knob (FxDlyDamp, "Damping");
                                     knob (FxDlyLowCut, "Low Cut"); knob (FxDlyMod, "Modulation"); knob (FxDlyDuck, "Ducking"); knob (FxDlyMix, "Mix"); break;
            case Module::Reverb:     seg (FxVerbMode, "Space", 5.0f); knob (FxVerbSize, "Size"); knob (FxVerbDecay, "Decay"); knob (FxVerbDamp, "Damping");
                                     knob (FxVerbShimmer, "Shimmer"); knob (FxVerbPre, "Pre-Delay"); knob (FxVerbLowCut, "Low Cut"); knob (FxVerbWidth, "Width");
                                     knob (FxVerbMod, "Modulation"); toggle (FxVerbFreeze, "Freeze", "Tail"); knob (FxVerbDuck, "Ducking"); knob (FxVerbMix, "Mix"); break;
            case Module::Width:      knob (FxWidthAmount, "Width"); knob (FxWidthMono, "Mono Below"); knob (FxWidthHaas, "Haas"); break;
            case Module::Tremolo:    seg (FxTremMode, "Mode", 3.5f, { "Tremolo", "Pan", "Harmonic" }); choice (FxTremShape, "Shape"); toggle (FxTremSync, "Sync", "Tempo");
                                     knob (FxTremRate, "Rate"); choice (FxTremNote, "Note"); knob (FxTremDepth, "Depth"); knob (FxTremStereo, "Stereo");
                                     knob (FxTremSmooth, "Smoothing"); knob (FxTremMix, "Mix"); break;
            case Module::Shifter:    seg (FxShiftMode, "Mode", 2.0f); knob (FxShiftAmount, "Shift"); knob (FxShiftFeedback, "Feedback"); knob (FxShiftSpread, "Stereo"); knob (FxShiftMix, "Mix"); break;
            case Module::Exciter:    knob (FxExcFreq, "Frequency"); knob (FxExcDrive, "Drive"); knob (FxExcColor, "Odd / Even"); knob (FxExcAmount, "Amount"); break;
            case Module::Harmonizer: knob (FxHarmPitch1, "Voice 1"); knob (FxHarmLevel1, "Level 1"); knob (FxHarmPitch2, "Voice 2"); knob (FxHarmLevel2, "Level 2");
                                     knob (FxHarmDetune, "Detune"); knob (FxHarmDelay, "Delay"); knob (FxHarmFeedback, "Feedback"); knob (FxHarmTone, "Tone"); knob (FxHarmMix, "Mix"); break;
            case Module::Out:        seg (FxOutMode, "Curve", 3.0f); knob (FxOutDrive, "Drive"); knob (FxOutCeiling, "Ceiling"); break;
            case Module::Count:
            default: break;
        }
    }

    // ---------------------------------------------------------------------------------------------
    // The chain as tiles in two rows: drag to reorder, click to edit, dot = on / off. Every tile shows its effect live.
    class Strip : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Strip (Host& h, std::array<Anim, kNumModules>& a) : host (h), anims (a)
        {
            setTooltip ("Drag to change the order, click to edit, the dot switches an effect on / off");
        }

        std::function<void (int)> onSelect;
        int selected = 0;

        void setValues (Values v) { values = v; repaint(); }

        static constexpr int perRow = 8;

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            const auto order = currentOrder();
            // Signal flow lines behind the tiles
            g.setColour (p.line);
            for (int row = 0; row < 2; ++row)
                g.fillRect (0.0f, rowY (row) + tileHeight() * 0.5f, (float) getWidth(), 1.0f);

            for (int slot = 0; slot < numTiles; ++slot)
            {
                const int type = typeAt (order, slot);
                auto r = tileBounds (slot);
                if (dragType == type && dragOrder.has_value())
                    r = r.withPosition (juce::jlimit (0.0f, (float) getWidth() - r.getWidth(), dragPos.x - grabOffset.x),
                                        juce::jlimit (0.0f, (float) getHeight() - r.getHeight(), dragPos.y - grabOffset.y));
                drawTile (g, type, slot, r);
            }
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const int s = slotAt (e.position);
            if (s != hover) { hover = s; repaint(); }
            setMouseCursor (s >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        }

        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            const int s = slotAt (e.position);
            if (s < 0) return;
            const int type = typeAt (currentOrder(), s);
            if (powerBounds (tileBounds (s)).expanded (4.0f).contains (e.position))
            {
                if (auto* param = host.state.getParameter (paramId (onParam (type))))
                {
                    if (host.undo != nullptr) host.undo->beginNewTransaction ("Effect on / off");
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f);
                    param->endChangeGesture();
                }
                return;
            }
            dragType = type == kOut || host.getOrder == nullptr ? -1 : type;
            grabOffset = e.position - tileBounds (s).getPosition();
            selected = type;
            if (onSelect) onSelect (type);
            repaint();
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragType < 0 || e.getDistanceFromDragStart() < 4) return;
            dragPos = e.position;
            auto order = dragOrder.has_value() ? *dragOrder : host.getOrder();
            // Move the dragged module to the slot under its centre
            const auto centre = e.position - grabOffset + juce::Point<float> (tileWidth() * 0.5f, tileHeight() * 0.5f);
            const int target = juce::jlimit (0, kNumChain - 1, slotAt (centre, true));
            auto it = std::find (order.begin(), order.end(), dragType);
            const int from = (int) std::distance (order.begin(), it);
            if (from != target && from < kNumChain)
            {
                const int t = order[(size_t) from];
                if (from < target) for (int i = from; i < target; ++i) order[(size_t) i] = order[(size_t) i + 1];
                else               for (int i = from; i > target; --i) order[(size_t) i] = order[(size_t) i - 1];
                order[(size_t) target] = t;
            }
            dragOrder = order;
            repaint();
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            if (dragOrder.has_value() && host.setOrder != nullptr && *dragOrder != host.getOrder())
            {
                if (host.undo != nullptr) host.undo->beginNewTransaction ("Effect order");
                host.setOrder (*dragOrder);
            }
            dragOrder.reset();
            dragType = -1;
            repaint();
        }

    private:
        static constexpr int numTiles = kNumModules;
        static constexpr float gap = 8.0f;

        Order currentOrder() const
        {
            if (dragOrder.has_value()) return *dragOrder;
            return host.getOrder != nullptr ? host.getOrder() : defaultOrder();
        }

        static int typeAt (const Order& order, int slot) { return slot < kNumChain ? order[(size_t) slot] : kOut; }
        float tileWidth() const { return ((float) getWidth() - (float) (perRow - 1) * gap) / (float) perRow; }
        float tileHeight() const { return ((float) getHeight() - gap) * 0.5f; }
        float rowY (int row) const { return (float) row * (tileHeight() + gap); }
        juce::Rectangle<float> tileBounds (int slot) const
        {
            return { (float) (slot % perRow) * (tileWidth() + gap), rowY (slot / perRow), tileWidth(), tileHeight() };
        }
        static juce::Rectangle<float> powerBounds (juce::Rectangle<float> tile) { return { tile.getRight() - 20.0f, tile.getY() + 6.0f, 14.0f, 14.0f }; }

        int slotAt (juce::Point<float> pos, bool clampToGrid = false) const
        {
            if (! clampToGrid && ! getLocalBounds().toFloat().contains (pos)) return -1;
            const int col = juce::jlimit (0, perRow - 1, (int) (pos.x / (tileWidth() + gap)));
            const int row = juce::jlimit (0, 1, (int) (pos.y / (tileHeight() + gap)));
            return juce::jlimit (0, numTiles - 1, row * perRow + col);
        }

        void drawTile (juce::Graphics& g, int type, int slot, juce::Rectangle<float> r)
        {
            const auto& p = Theme::get().palette();
            const bool on = values.v != nullptr && values.b (onParam (type));
            const bool sel = type == selected;
            r = r.reduced (0.5f);
            g.setColour (p.panel);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (sel ? p.accent : (slotAt (r.getCentre()) == hover ? p.textDim : p.line));
            g.drawRoundedRectangle (r, 6.0f, sel ? 1.4f : 1.0f);

            g.setColour (on ? p.text : p.textDim);
            drawSpacedText (g, juce::String (info (type).tile).toUpperCase(), juce::Rectangle<float> (r.getX() + 9.0f, r.getY() + 4.0f, r.getWidth() - 30.0f, 18.0f),
                            8.5f, juce::Justification::centredLeft, 0.12f, sel);

            const auto dot = powerBounds (r).reduced (2.5f);
            if (on) { g.setColour (p.accent); g.fillEllipse (dot); }
            else    { g.setColour (p.textDim); g.drawEllipse (dot.reduced (0.5f), 1.2f); }

            auto graph = r.reduced (9.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (6.0f);
            if (values.v != nullptr)
            {
                juce::Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (graph.toNearestInt());
                if (! on) g.setOpacity (0.5f);
                drawGraph (g, type, graph, values, host.meters, anims[(size_t) type], true);
            }
            if (type != kOut)
            {
                g.setColour (p.textDim.withAlpha (0.7f));
                g.setFont (font (8.5f, Weight::Medium));
                g.drawText (juce::String (slot + 1), juce::Rectangle<float> (r.getX() + 9.0f, r.getBottom() - 15.0f, 24.0f, 12.0f),
                            juce::Justification::centredLeft);
            }
        }

        Host& host;
        std::array<Anim, kNumModules>& anims;
        Values values;
        std::optional<Order> dragOrder;
        int dragType = -1, hover = -1;
        juce::Point<float> dragPos, grabOffset;
    };

    // ---------------------------------------------------------------------------------------------
    // Big graph of the selected effect. The EQ graph is interactive: drag the band points, the wheel sets Q.
    class GraphPanel : public juce::Component
    {
    public:
        GraphPanel (Host& h, Anim& a) : host (h), anim (a) {}

        int type = 0;

        void setValues (Values v) { values = v; repaint(); }
        void setType (int t) { type = t; anim = {}; repaint(); }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            const bool dark = Theme::get().isDark();
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setGradientFill (juce::ColourGradient (dark ? p.background.brighter (0.03f) : p.panel, 0.0f, r.getY(),
                                                     dark ? p.panel : p.background, 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (p.line);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
            if (values.v == nullptr) return;
            const auto area = graphArea();
            {
                juce::Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (area.expanded (2.0f).toNearestInt());
                drawGraph (g, type, area, values, host.meters, anim, false);
            }
            if (type == (int) Module::Eq) drawEqHandles (g, area);
            if (type == (int) Module::Eq || type == (int) Module::Filter || type == (int) Module::Shifter || type == (int) Module::Exciter)
            {
                g.setColour (p.textDim);
                g.setFont (font (9.0f, Weight::Medium));
                for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
                    g.drawText (hz >= 1000.0f ? juce::String ((int) (hz / 1000.0f)) + "k" : juce::String ((int) hz),
                                juce::Rectangle<float> (ui::freqToX (hz, area) - 20.0f, area.getBottom() + 4.0f, 40.0f, 12.0f), juce::Justification::centred);
            }
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            dragBand = type == (int) Module::Eq ? bandAt (e.position) : -1;
            if (dragBand >= 0)
            {
                if (host.undo != nullptr) host.undo->beginNewTransaction ("EQ band");
                for (auto id : bandParams (dragBand)) if (id != NumParams) param (id)->beginChangeGesture();
            }
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragBand < 0) return;
            const auto area = graphArea();
            const auto ids = bandParams (dragBand);
            setPlain (ids[0], juce::jlimit (20.0f, 20000.0f, ui::xToFreq (e.position.x, area)));
            if (ids[1] != NumParams)
                setPlain (ids[1], juce::jlimit (-18.0f, 18.0f, (area.getCentreY() - e.position.y) / (area.getHeight() * 0.5f) * 20.0f));
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            if (dragBand >= 0)
                for (auto id : bandParams (dragBand)) if (id != NumParams) param (id)->endChangeGesture();
            dragBand = -1;
        }

        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
        {
            const int b = type == (int) Module::Eq ? bandAt (e.position) : -1;
            if (b == 2 || b == 3)
            {
                auto* q = param (b == 2 ? FxEqMidQ : FxEqMid2Q);
                q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, q->getValue() + w.deltaY * 0.15f));
            }
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const int b = type == (int) Module::Eq ? bandAt (e.position) : -1;
            if (b != hoverBand) { hoverBand = b; repaint(); }
            setMouseCursor (b >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        }

    private:
        juce::Rectangle<float> graphArea() const { return getLocalBounds().toFloat().reduced (20.0f, 18.0f).withTrimmedBottom (14.0f); }
        juce::RangedAudioParameter* param (Param p) const { return host.state.getParameter (paramId (p)); }

        // Bands: 0 low cut, 1 low shelf, 2 mid, 3 mid 2, 4 high shelf, 5 high cut  -> { freq, gain }
        static std::array<Param, 2> bandParams (int b)
        {
            switch (b)
            {
                case 0:  return { FxEqLowCut, NumParams };
                case 1:  return { FxEqLowFreq, FxEqLowGain };
                case 2:  return { FxEqMidFreq, FxEqMidGain };
                case 3:  return { FxEqMid2Freq, FxEqMid2Gain };
                case 4:  return { FxEqHighFreq, FxEqHighGain };
                default: return { FxEqHighCut, NumParams };
            }
        }

        juce::Point<float> bandPos (int b, juce::Rectangle<float> area) const
        {
            const auto ids = bandParams (b);
            const float x = ui::freqToX (values[ids[0]], area);
            const float db = ids[1] != NumParams ? values[ids[1]] : 0.0f;
            return { x, area.getCentreY() - db / 20.0f * area.getHeight() * 0.5f };
        }

        int bandAt (juce::Point<float> pos) const
        {
            if (values.v == nullptr) return -1;
            const auto area = graphArea();
            for (int b = 0; b < 6; ++b)
                if (bandPos (b, area).getDistanceFrom (pos) < 10.0f) return b;
            return -1;
        }

        void drawEqHandles (juce::Graphics& g, juce::Rectangle<float> area)
        {
            const auto& p = Theme::get().palette();
            static const char* names[] = { "LC", "L", "M", "M2", "H", "HC" };
            for (int b = 0; b < 6; ++b)
            {
                const auto pos = bandPos (b, area);
                if (b == hoverBand || b == dragBand)
                {
                    g.setColour (p.accent.withAlpha (0.2f));
                    g.fillEllipse (juce::Rectangle<float> (26.0f, 26.0f).withCentre (pos));
                }
                g.setColour (p.panel);
                g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (pos));
                g.setColour (p.accent);
                g.drawEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (pos), 1.4f);
                g.setColour (p.text);
                g.setFont (font (8.0f, Weight::SemiBold));
                g.drawText (names[b], juce::Rectangle<float> (16.0f, 16.0f).withCentre (pos), juce::Justification::centred);
            }
        }

        void setPlain (Param id, float v)
        {
            auto* p = param (id);
            p->setValueNotifyingHost (p->convertTo0to1 (v));
        }

        Host& host;
        Anim& anim;
        Values values;
        int dragBand = -1, hoverBand = -1;
    };

    // ---------------------------------------------------------------------------------------------
    // Title, description, preset menu and on switch of the selected effect, its controls below.
    class ModuleEditor : public juce::Component, private juce::Timer
    {
    public:
        explicit ModuleEditor (Host& h) : host (h)
        {
            for (int t = 0; t < kNumModules; ++t)
            {
                if (h.state.getParameter (paramId (onParam (t))) == nullptr)
                    continue;   // a plugin with only some effects (the single-effect plugins)
                buildControls (t, h.state, controls[(size_t) t]);
                controls[(size_t) t].addTo (*this);
                power[(size_t) t] = std::make_unique<Toggle> (h.state, paramId (onParam (t)), "On");
                addChildComponent (*power[(size_t) t]);
            }
            addAndMakeVisible (presetMenu);
            presetMenu.setTooltip ("Presets for this effect only - the rest of the chain stays as it is");
            presetMenu.onChange = [this] { presetChosen(); };
            select (0);
            startTimerHz (4);
        }

        // Single-effect plugins hide the on switch (the plugin has its own bypass)
        bool showPower = true;

        void select (int type)
        {
            selected = juce::jlimit (0, kNumModules - 1, type);
            for (int t = 0; t < kNumModules; ++t)
            {
                controls[(size_t) t].setVisible (t == selected);
                if (power[(size_t) t] != nullptr) power[(size_t) t]->setVisible (showPower && t == selected);
            }
            shownPreset = -1;
            refreshPresetMenu();
            resized();
            repaint();
        }

        int getSelected() const { return selected; }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (p.panel);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (p.line);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
            g.setColour (p.text);
            drawSpacedText (g, juce::String (info (selected).name).toUpperCase(), r.reduced (16.0f, 0.0f).removeFromTop (38.0f), 12.0f,
                            juce::Justification::centredLeft, 0.25f, true);
            g.setColour (p.textDim);
            drawSpacedText (g, info (selected).description, r.reduced (16.0f, 0.0f).withTrimmedTop (30.0f).removeFromTop (18.0f), 9.5f,
                            juce::Justification::centredLeft, 0.06f);
        }

        void resized() override
        {
            auto e = getLocalBounds().reduced (16, 0);
            auto head = e.removeFromTop (40);
            if (showPower && power[(size_t) selected] != nullptr)
                power[(size_t) selected]->setBounds (head.removeFromRight (56).withSizeKeepingCentre (56, 20));
            head.removeFromRight (10);
            presetMenu.setBounds (head.removeFromRight (std::min (220, head.getWidth() / 2)).withSizeKeepingCentre (std::min (220, head.getWidth() / 2), 24));
            e.removeFromTop (14);

            // Rows of controls: more columns when the editor is wide, row height shrinks to fit
            auto& row = controls[(size_t) selected];
            const int perRow = e.getWidth() >= 560 ? 6 : (e.getWidth() >= 420 ? 5 : 4);
            int units = 0, rows = 1, used = 0;
            for (auto& [c, u] : row.items)
            {
                const int w = juce::jlimit (1, perRow, (int) std::ceil (u - 0.25f));
                if (used + w > perRow) { ++rows; used = 0; }
                used += w;
                units += w;
            }
            const int knobRow = juce::jlimit (60, 84, (e.getHeight() - 4 * rows) / std::max (1, rows));
            row.layoutGrid (e, perRow, knobRow, std::min (48, knobRow));
        }

    private:
        void timerCallback() override { if (isShowing()) updatePresetName(); }

        static constexpr int saveId = 10000, userIdBase = 5000;

        void refreshPresetMenu()
        {
            presetMenu.clear (juce::dontSendNotification);
            const auto& list = ModulePresets::get().getPresets (selected);
            juce::String section;
            for (int i = 0; i < (int) list.size(); ++i)
            {
                if (list[(size_t) i].category != section)
                {
                    section = list[(size_t) i].category;
                    presetMenu.addSectionHeading (section);
                }
                presetMenu.addItem (list[(size_t) i].name, (list[(size_t) i].file == juce::File() ? 1 : userIdBase) + i);
            }
            presetMenu.addSeparator();
            presetMenu.addItem (ui::utf8 ("Save current settings \xe2\x80\xa6"), saveId);
            updatePresetName (true);
        }

        // The menu shows the name of the preset the current settings match, otherwise "<Effect> presets".
        void updatePresetName (bool force = false)
        {
            const auto& list = ModulePresets::get().getPresets (selected);
            int match = -1;
            if (shownPreset >= 0 && shownPreset < (int) list.size() && ModulePresets::matches (host.state, selected, list[(size_t) shownPreset]))
                match = shownPreset;
            else
                for (int i = 0; i < (int) list.size() && match < 0; ++i)
                    if (ModulePresets::matches (host.state, selected, list[(size_t) i]))
                        match = i;
            if (match == shownPreset && ! force) return;
            shownPreset = match;
            presetMenu.setTextWhenNothingSelected (match >= 0 ? list[(size_t) match].name : juce::String (info (selected).name) + " presets");
        }

        void presetChosen()
        {
            const int id = presetMenu.getSelectedId();
            presetMenu.setSelectedId (0, juce::dontSendNotification);
            if (id == saveId) return askForName();
            const int index = id >= userIdBase ? id - userIdBase : id - 1;
            const auto& list = ModulePresets::get().getPresets (selected);
            if (juce::isPositiveAndBelow (index, (int) list.size()))
            {
                ModulePresets::apply (host.state, selected, list[(size_t) index], host.undo);
                shownPreset = index;
                updatePresetName (true);
            }
        }

        void askForName()
        {
            auto* w = new juce::AlertWindow ("Save " + juce::String (info (selected).name) + " preset",
                                             "The preset will be available in every Fjord plugin.", juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", "", "Name");
            w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create (
                [w, safe = juce::Component::SafePointer<ModuleEditor> (this)] (int r)
                {
                    if (r == 1 && safe != nullptr)
                    {
                        ModulePresets::get().saveUserPreset (safe->host.state, safe->selected, w->getTextEditorContents ("name"));
                        safe->refreshPresetMenu();
                    }
                }), true);
        }

        Host& host;
        std::array<ControlRow, kNumModules> controls;
        std::array<std::unique_ptr<Toggle>, kNumModules> power;
        juce::ComboBox presetMenu;
        int selected = 0, shownPreset = -1;
    };

    // ---------------------------------------------------------------------------------------------
    // Bar at the top of the FX page:  FX CHAIN  ‹ Big Hall ›  ⤓              ● LOCK FX
    class ChainBar : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit ChainBar (ChainPresets& m) : fx (m) { startTimerHz (5); }

        std::function<void()> onOpenPicker, onSave;

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            g.setColour (p.textDim);
            drawSpacedText (g, "FX CHAIN", labelArea().toFloat(), 10.5f, juce::Justification::centredLeft, 0.22f);

            auto bar = barArea().toFloat().reduced (0.5f);
            g.setColour (p.panel);
            g.fillRoundedRectangle (bar, bar.getHeight() * 0.5f);
            g.setColour (hover == 2 ? p.textDim : p.line);
            g.drawRoundedRectangle (bar, bar.getHeight() * 0.5f, 1.0f);

            auto chevron = [&] (juce::Rectangle<int> a, bool left, bool hot)
            {
                const auto c = a.toFloat().getCentre();
                const float d = left ? 1.0f : -1.0f;
                juce::Path path;
                path.startNewSubPath (c.x + 2.0f * d, c.y - 4.0f);
                path.lineTo (c.x - 2.0f * d, c.y);
                path.lineTo (c.x + 2.0f * d, c.y + 4.0f);
                g.setColour (hot ? p.accent : p.textDim);
                g.strokePath (path, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };
            chevron (prevArea(), true, hover == 1);
            chevron (nextArea(), false, hover == 3);

            const auto name = fx.getCurrentName();
            g.setColour (name.isEmpty() ? p.textDim : p.text);
            g.setFont (font (13.5f));
            g.drawText (name.isEmpty() ? juce::String ("From sound preset") : name, nameArea(), juce::Justification::centred);

            // Save icon
            {
                const auto c = saveArea().toFloat().getCentre();
                juce::Path icon;
                icon.startNewSubPath (c.x, c.y - 5.0f); icon.lineTo (c.x, c.y + 2.0f);
                icon.startNewSubPath (c.x - 3.0f, c.y - 1.0f); icon.lineTo (c.x, c.y + 2.0f); icon.lineTo (c.x + 3.0f, c.y - 1.0f);
                icon.startNewSubPath (c.x - 5.0f, c.y + 2.0f); icon.lineTo (c.x - 5.0f, c.y + 5.0f);
                icon.lineTo (c.x + 5.0f, c.y + 5.0f); icon.lineTo (c.x + 5.0f, c.y + 2.0f);
                g.setColour (hover == 4 ? p.accent : p.textDim);
                g.strokePath (icon, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Lock switch
            const bool locked = fx.isLocked();
            auto lock = lockArea().toFloat();
            const auto dot = lock.removeFromLeft (16.0f).withSizeKeepingCentre (12.0f, 12.0f);
            g.setColour (locked ? p.accent : p.line);
            if (locked) g.fillEllipse (dot); else g.drawEllipse (dot.reduced (0.5f), 1.2f);
            g.setColour (locked ? p.text : p.textDim);
            drawSpacedText (g, "LOCK FX", lock.withTrimmedLeft (6.0f), 10.5f, juce::Justification::centredLeft, 0.18f, locked);
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const int z = zoneAt (e.getPosition());
            if (z != hover)
            {
                hover = z;
                setTooltip (z == 5 ? "Keep these effects when switching sound presets"
                          : z == 4 ? "Save effect chain" : z == 2 ? "Browse effect chains" : "");
                setMouseCursor (z > 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
                repaint();
            }
        }

        void mouseExit (const juce::MouseEvent&) override { hover = 0; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            switch (zoneAt (e.getPosition()))
            {
                case 1: fx.previous(); break;
                case 2: if (onOpenPicker) onOpenPicker(); break;
                case 3: fx.next(); break;
                case 4: if (onSave) onSave(); break;
                case 5: fx.setLocked (! fx.isLocked()); break;
                default: break;
            }
            repaint();
        }

    private:
        juce::Rectangle<int> labelArea() const { return getLocalBounds().withWidth (90); }
        juce::Rectangle<int> barArea() const { return getLocalBounds().withTrimmedLeft (90).withWidth (330).reduced (0, 4); }
        juce::Rectangle<int> prevArea() const { return barArea().withWidth (30); }
        juce::Rectangle<int> nextArea() const { return barArea().withTrimmedRight (34).removeFromRight (30); }
        juce::Rectangle<int> saveArea() const { return barArea().removeFromRight (34); }
        juce::Rectangle<int> nameArea() const { return barArea().withTrimmedLeft (30).withTrimmedRight (64); }
        juce::Rectangle<int> lockArea() const { return getLocalBounds().removeFromRight (110); }

        int zoneAt (juce::Point<int> pos) const
        {
            if (prevArea().contains (pos)) return 1;
            if (nextArea().contains (pos)) return 3;
            if (saveArea().contains (pos)) return 4;
            if (barArea().contains (pos)) return 2;
            if (lockArea().contains (pos)) return 5;
            return 0;
        }

        void timerCallback() override
        {
            const auto key = fx.getCurrentName() + (fx.isLocked() ? "1" : "0");
            if (key != shown) { shown = key; repaint(); }
        }

        ChainPresets& fx;
        int hover = 0;
        juce::String shown;
    };

    // Effect chain picker: one column per group.
    class ChainPicker : public juce::Component
    {
    public:
        explicit ChainPicker (ChainPresets& m) : fx (m), done ("Done", false)
        {
            addAndMakeVisible (done);
            done.onClick = [this] { setVisible (false); };
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            auto r = getLocalBounds().toFloat();
            g.setColour (p.panel);
            g.fillRoundedRectangle (r, 8.0f);
            g.setColour (p.line);
            g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

            const auto cats = categories();
            for (int c = 0; c < cats.size(); ++c)
            {
                g.setColour (p.text);
                drawSpacedText (g, cats[c].toUpperCase(), columnArea (c, cats.size()).removeFromTop (30).toFloat(),
                                11.0f, juce::Justification::centredLeft, 0.25f, true);
            }

            for (const auto& item : items())
            {
                const bool cur = item.index == fx.getCurrentIndex(), hov = item.index == hovered;
                auto ir = item.bounds.toFloat();
                if (cur || hov)
                {
                    g.setColour (cur ? p.accentSoft : p.track.withAlpha (0.6f));
                    g.fillRoundedRectangle (ir, 5.0f);
                }
                g.setColour (cur ? p.accent : p.text);
                g.setFont (font (13.0f, cur));
                g.drawText (fx.getPresets()[(size_t) item.index].name, ir.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
            }
        }

        void resized() override { done.setBounds (getLocalBounds().removeFromBottom (48).removeFromRight (120).reduced (12, 8)); }

        void mouseMove (const juce::MouseEvent& e) override
        {
            int h = -1;
            for (const auto& item : items()) if (item.bounds.contains (e.getPosition())) h = item.index;
            if (h != hovered) { hovered = h; repaint(); }
            setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            for (const auto& item : items())
                if (item.bounds.contains (e.getPosition()))
                {
                    fx.load (item.index);
                    repaint();
                }
        }

        void mouseDoubleClick (const juce::MouseEvent&) override { setVisible (false); }

    private:
        struct Item { int index; juce::Rectangle<int> bounds; };

        juce::StringArray categories() const
        {
            juce::StringArray c;
            for (const auto& p : fx.getPresets()) c.addIfNotAlreadyThere (p.category);
            return c;
        }

        juce::Rectangle<int> columnArea (int c, int count) const
        {
            auto r = getLocalBounds().reduced (20, 14).withTrimmedBottom (40);
            const int w = r.getWidth() / std::max (1, count);
            return r.withX (r.getX() + c * w).withWidth (w - 10);
        }

        std::vector<Item> items() const
        {
            std::vector<Item> out;
            const auto cats = categories();
            std::vector<int> row ((size_t) cats.size(), 0);
            const auto& list = fx.getPresets();
            for (int i = 0; i < (int) list.size(); ++i)
            {
                const int c = cats.indexOf (list[(size_t) i].category);
                auto col = columnArea (c, cats.size()).withTrimmedTop (32);
                out.push_back ({ i, col.withY (col.getY() + row[(size_t) c]++ * 30).withHeight (28) });
            }
            return out;
        }

        ChainPresets& fx;
        PillButton done;
        int hovered = -1;
    };

    // Small dialog for naming an effect chain.
    class ChainSavePrompt : public juce::Component
    {
    public:
        explicit ChainSavePrompt (ChainPresets& m) : fx (m), save ("Save", true), cancel ("Cancel", false)
        {
            name.setTextToShowWhenEmpty ("Effect chain name", juce::Colours::grey);
            name.onReturnKey = [this] { commit(); };
            name.onEscapeKey = [this] { setVisible (false); };
            for (auto* c : std::initializer_list<juce::Component*> { &name, &save, &cancel }) addAndMakeVisible (c);
            save.onClick = [this] { commit(); };
            cancel.onClick = [this] { setVisible (false); };
        }

        void open()
        {
            styleTextEditor (name);
            name.setText (fx.getCurrentName(), false);
            setVisible (true);
            toFront (true);
            name.grabKeyboardFocus();
            name.selectAll();
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = Theme::get().palette();
            g.fillAll (p.background.withAlpha (0.8f));
            const auto c = card().toFloat();
            g.setColour (p.panel);
            g.fillRoundedRectangle (c, 12.0f);
            g.setColour (p.line);
            g.drawRoundedRectangle (c, 12.0f, 1.0f);
            g.setColour (p.text);
            drawSpacedText (g, "SAVE EFFECT CHAIN", card().reduced (24, 20).removeFromTop (20).toFloat(), 12.0f,
                            juce::Justification::centredLeft, 0.25f, true);
        }

        void resized() override
        {
            auto r = card().reduced (24, 20);
            r.removeFromTop (34);
            name.setBounds (r.removeFromTop (38));
            auto buttons = r.removeFromBottom (32);
            save.setBounds (buttons.removeFromRight (110));
            buttons.removeFromRight (10);
            cancel.setBounds (buttons.removeFromRight (110));
        }

        void mouseDown (const juce::MouseEvent& e) override { if (! card().contains (e.getPosition())) setVisible (false); }

    private:
        juce::Rectangle<int> card() const { return getLocalBounds().withSizeKeepingCentre (440, 180); }

        void commit()
        {
            if (fx.saveUserPreset (name.getText())) setVisible (false);
        }

        ChainPresets& fx;
        juce::TextEditor name;
        PillButton save, cancel;
    };

    // ---------------------------------------------------------------------------------------------
    // The effect page of every plugin: the chain as tiles, the big graph and the editor of the selected effect.
    // A plugin may put its own bar on top (e.g. effect chain presets).
    class FxPage : public ui::Page
    {
    public:
        explicit FxPage (Host h) : host (std::move (h)), uiValues (host.state), strip (host, anims), graph (host, graphAnim), editor (host)
        {
            addAndMakeVisible (strip);
            addAndMakeVisible (graph);
            addAndMakeVisible (editor);
            strip.onSelect = [this] (int t) { selectEffect (t); };
            if (host.chains != nullptr)
            {
                chainBar = std::make_unique<ChainBar> (*host.chains);
                chainPicker = std::make_unique<ChainPicker> (*host.chains);
                chainPrompt = std::make_unique<ChainSavePrompt> (*host.chains);
                addAndMakeVisible (*chainBar);
                addChildComponent (*chainPicker);
                addChildComponent (*chainPrompt);
                chainBar->onOpenPicker = [this] { chainPicker->setVisible (! chainPicker->isVisible()); chainPicker->toFront (false); };
                chainBar->onSave = [this] { chainPicker->setVisible (false); chainPrompt->open(); };
            }
            selectEffect ((int) Module::Reverb);
        }

        void setTopBar (juce::Component* bar, int height)
        {
            topBar = bar;
            topBarHeight = height;
            if (bar != nullptr) addAndMakeVisible (bar);
            resized();
        }

        void selectEffect (int type)
        {
            strip.selected = type;
            graph.setType (type);
            editor.select (type);
            repaint();
        }

        int getSelected() const { return editor.getSelected(); }

        void tick() override
        {
            uiValues.update();
            const auto v = uiValues.view();
            for (int t = 0; t < kNumModules; ++t) anims[(size_t) t].tick (host.meters, v, 1.0f / 30.0f, t);
            graphAnim.tick (host.meters, v, 1.0f / 30.0f, graph.type);
            strip.setValues (v);
            graph.setValues (v);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            if (chainBar != nullptr)
            {
                chainBar->setBounds (r.removeFromTop (32));
                r.removeFromTop (8);
                chainPicker->setBounds (r);
                chainPrompt->setBounds (getLocalBounds());
            }
            if (topBar != nullptr)
            {
                topBar->setBounds (r.removeFromTop (topBarHeight));
                r.removeFromTop (8);
            }
            strip.setBounds (r.removeFromTop (juce::jlimit (110, 150, r.getHeight() / 3)));
            if (chainPicker != nullptr) { chainPicker->toFront (false); chainPrompt->toFront (false); }
            r.removeFromTop (12);
            graph.setBounds (r.removeFromLeft ((int) ((float) r.getWidth() * 0.42f)));
            r.removeFromLeft (12);
            editor.setBounds (r);
        }

    private:
        Host host;
        UiValues uiValues;
        std::array<Anim, kNumModules> anims;
        Anim graphAnim;
        Strip strip;
        GraphPanel graph;
        ModuleEditor editor;
        std::unique_ptr<ChainBar> chainBar;
        std::unique_ptr<ChainPicker> chainPicker;
        std::unique_ptr<ChainSavePrompt> chainPrompt;
        juce::Component* topBar = nullptr;
        int topBarHeight = 0;
    };
}
