#pragma once
#include "Effects.h"

namespace fjord::base::fx
{
    // Lock-free values for the live visuals on the effect tiles (written by the audio thread, read by the UI).
    struct Meters
    {
        std::array<std::atomic<float>, kNumModules> in {}, out {};
        std::atomic<float> compReduction { 0.0f }, transientGain { 0.0f }, filterEnv { 0.0f }, filterCutoff { 1000.0f },
                           squash { 0.0f }, reverbTail { 0.0f }, motionPhase { 0.0f }, lofiWow { 0.0f }, clipOver { 0.0f },
                           tremoloPhase { 0.0f }, tremoloLevel { 0.5f }, shifterPhase { 0.0f }, exciterLevel { 0.0f };
        std::atomic<uint32_t> triggers { 0 };   // counts notes / hits seen by the chain (graphs react to them)
        std::atomic<double> bpm { 120.0 };
        std::atomic<double> sampleRate { 48000.0 };
    };

    // All effect modules in a user-defined order, the clipper always last.
    class Chain
    {
    public:
        void prepare (double sr, int maxBlock)
        {
            sampleRate = sr;
            sat.prepare (sr, maxBlock);
            lofi.prepare (sr, maxBlock);
            trans.prepare (sr, maxBlock);
            comp.prepare (sr, maxBlock);
            squash.prepare (sr, maxBlock);
            eq.prepare (sr, maxBlock);
            filter.prepare (sr, maxBlock);
            motion.prepare (sr, maxBlock);
            delay.prepare (sr, maxBlock);
            reverb.prepare (sr, maxBlock);
            width.prepare (sr, maxBlock);
            tremolo.prepare (sr, maxBlock);
            shifter.prepare (sr, maxBlock);
            exciter.prepare (sr, maxBlock);
            harmonizer.prepare (sr, maxBlock);
            clipper.prepare (sr, maxBlock);
            wasOn.fill (false);
        }

        void reset()
        {
            for (int m = 0; m < kNumModules; ++m)
                resetModule (m);
        }

        void resetModule (int m)
        {
            switch ((Module) m)
            {
                case Module::Saturation: sat.reset(); break;
                case Module::Lofi:       lofi.reset(); break;
                case Module::Transient:  trans.reset(); break;
                case Module::Compressor: comp.reset(); break;
                case Module::Squash:     squash.reset(); break;
                case Module::Eq:         eq.reset(); break;
                case Module::Filter:     filter.reset(); break;
                case Module::Motion:     motion.reset(); break;
                case Module::Delay:      delay.reset(); break;
                case Module::Reverb:     reverb.reset(); break;
                case Module::Width:      width.reset(); break;
                case Module::Tremolo:    tremolo.reset(); break;
                case Module::Shifter:    shifter.reset(); break;
                case Module::Exciter:    exciter.reset(); break;
                case Module::Harmonizer: harmonizer.reset(); break;
                case Module::Out:        clipper.reset(); break;
                case Module::Count:
                default: break;
            }
        }

        // Processes one module (whatever the order); used by the single-effect plugins.
        void processModule (int m, StereoBuffer b, const Values& v, const Context& c)
        {
            switch ((Module) m)
            {
                case Module::Saturation: sat.process (b, v, c); break;
                case Module::Lofi:       lofi.process (b, v, c); break;
                case Module::Transient:  trans.process (b, v, c); break;
                case Module::Compressor: comp.process (b, v, c); break;
                case Module::Squash:     squash.process (b, v, c); break;
                case Module::Eq:         eq.process (b, v, c); break;
                case Module::Filter:     filter.process (b, v, c); break;
                case Module::Motion:     motion.process (b, v, c); break;
                case Module::Delay:      delay.process (b, v, c); break;
                case Module::Reverb:     reverb.process (b, v, c); break;
                case Module::Width:      width.process (b, v, c); break;
                case Module::Tremolo:    tremolo.process (b, v, c); break;
                case Module::Shifter:    shifter.process (b, v, c); break;
                case Module::Exciter:    exciter.process (b, v, c); break;
                case Module::Harmonizer: harmonizer.process (b, v, c); break;
                case Module::Out:        clipper.process (b, v, c); break;
                case Module::Count:
                default: break;
            }
        }

        void process (StereoBuffer b, const Values& v, const Order& order, const Context& c, Meters* meters = nullptr)
        {
            for (int m : order)
                runModule (m, b, v, c, meters);
            runModule (kOut, b, v, c, meters);
            if (meters != nullptr)
                publish (*meters, v, c);
        }

        // For the single-effect plugins: one module with meters
        void processSingle (int m, StereoBuffer b, const Values& v, const Context& c, Meters* meters = nullptr)
        {
            runModule (m, b, v, c, meters, true);
            if (meters != nullptr)
                publish (*meters, v, c);
        }

    private:
        static float peak (StereoBuffer b)
        {
            float p = 0.0f;
            for (int i = 0; i < b.n; ++i) p = std::max (p, std::max (std::abs (b.l[i]), std::abs (b.r[i])));
            return p;
        }

        void runModule (int m, StereoBuffer b, const Values& v, const Context& c, Meters* meters, bool alwaysOn = false)
        {
            const bool on = alwaysOn || v.b (onParam (m));
            if (on && ! wasOn[(size_t) m]) resetModule (m);   // no stale tails when switched back on
            wasOn[(size_t) m] = on;
            if (! on)
            {
                if (meters != nullptr) { meters->in[(size_t) m].store (0.0f); meters->out[(size_t) m].store (0.0f); }
                return;
            }
            const float inPeak = meters != nullptr ? peak (b) : 0.0f;
            processModule (m, b, v, c);
            if (meters != nullptr)
            {
                meters->in[(size_t) m].store (inPeak, std::memory_order_relaxed);
                meters->out[(size_t) m].store (peak (b), std::memory_order_relaxed);
            }
        }

        void publish (Meters& m, const Values& v, const Context& c)
        {
            m.compReduction.store (v.b (FxCompOn) ? comp.lastReduction : 0.0f, std::memory_order_relaxed);
            m.transientGain.store (trans.lastGainDb, std::memory_order_relaxed);
            m.filterEnv.store (filter.currentEnv(), std::memory_order_relaxed);
            m.filterCutoff.store (filter.lastCutoff, std::memory_order_relaxed);
            m.squash.store (v.b (FxSqOn) ? squash.lastActivity : 0.0f, std::memory_order_relaxed);
            m.reverbTail.store (reverb.tailLevel, std::memory_order_relaxed);
            m.motionPhase.store (motion.getPhase(), std::memory_order_relaxed);
            m.lofiWow.store (lofi.getWow(), std::memory_order_relaxed);
            m.clipOver.store (clipper.lastOver, std::memory_order_relaxed);
            m.tremoloPhase.store (tremolo.getPhase(), std::memory_order_relaxed);
            m.tremoloLevel.store (tremolo.getLevel(), std::memory_order_relaxed);
            m.shifterPhase.store (shifter.lastPhase, std::memory_order_relaxed);
            m.exciterLevel.store (exciter.lastLevel, std::memory_order_relaxed);
            if (c.numTriggers > 0) m.triggers.fetch_add ((uint32_t) c.numTriggers, std::memory_order_relaxed);
            m.bpm.store (c.bpm, std::memory_order_relaxed);
            m.sampleRate.store (sampleRate, std::memory_order_relaxed);
        }

        double sampleRate = 48000.0;
        Saturation sat;
        Lofi lofi;
        TransientShaper trans;
        Compressor comp;
        Squash squash;
        Eq eq;
        Filter filter;
        Motion motion;
        Delay delay;
        Reverb reverb;
        Width width;
        Tremolo tremolo;
        FrequencyShifter shifter;
        Exciter exciter;
        Harmonizer harmonizer;
        Clipper clipper;
        std::array<bool, kNumModules> wasOn {};
    };
}
