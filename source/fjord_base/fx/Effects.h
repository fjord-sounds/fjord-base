#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../core/Dsp.h"
#include "FxParams.h"

// The DSP of every effect module. Each module reads plain values from fx::Values and processes a stereo buffer in
// place. No allocation and no locks in process(). Public statics (curves, designs) are shared with the graphs, so
// the pictures show exactly what the DSP does.
namespace fjord::base::fx
{
    using dsp::Smoother;
    using dsp::softClip;
    using dsp::kPi;
    using dsp::kTwoPi;
    using dsp::DelayLine;
    using dsp::Biquad;
    using dsp::StereoBuffer;
    using dsp::onePoleCoef;
    using dsp::equalPowerMix;

    // What the host and the instrument tell the effects for one block.
    struct Context
    {
        double bpm = 120.0;
        double ppq = 0.0;              // position at the block start (quarter notes)
        bool playing = false;          // host transport running: synced LFOs follow its position
        const int* triggers = nullptr; // sample positions of new notes / hits in this block (filter envelope)
        int numTriggers = 0;
    };

    // A free or tempo-synced LFO phase (0..1). While the host plays, synced phases follow the song position.
    struct LfoPhase
    {
        double phase = 0.0;

        // Returns the per-sample increment for this block and aligns the phase to the host if synced.
        double begin (bool sync, float rateHz, int note, const Context& c, double sampleRate)
        {
            if (sync)
            {
                const double beats = noteBeats (note);
                if (c.playing)
                    phase = std::fmod (c.ppq / beats, 1.0);
                return c.bpm / 60.0 / beats / sampleRate;
            }
            return rateHz / sampleRate;
        }

        float advance (double inc)
        {
            phase += inc;
            if (phase >= 1.0) phase -= std::floor (phase);
            return (float) phase;
        }
    };

    // ---------------------------------------------------------------------------------------------
    // Saturation: six transfer curves, bias, tilt tone, 1x / 2x / 4x oversampling, optional auto gain

    class Saturation
    {
    public:
        void prepare (double sr, int maxBlock)
        {
            sampleRate = (float) sr;
            for (auto& o : oversamplers) o->initProcessing ((size_t) maxBlock);
            dry.setSize (2, maxBlock);
            mix.setTime (0.02f, sampleRate);
            autoGain.setTime (0.3f, sampleRate);
            reset();
        }

        void reset()
        {
            for (auto& o : oversamplers) o->reset();
            tilt.fill (0.0f);
            lastGain = -1.0f;
            inRms = outRms = 0.0f;
            autoGain.reset (1.0f);
        }

        // The transfer curve, shared with the UI graph.
        static float shape (int mode, float x, float bias)
        {
            const float b = bias * 0.5f;
            switch (mode)
            {
                case 0:  return std::tanh (x + b) - std::tanh (b);                                             // Tape
                case 1:  { const float y = x + b;                                                              // Tube: asymmetric
                           return (y >= 0.0f ? 1.0f - std::exp (-y) : -(1.0f - std::exp (1.4f * y)) / 1.4f)
                                  - (b >= 0.0f ? 1.0f - std::exp (-b) : -(1.0f - std::exp (1.4f * b)) / 1.4f); }
                case 2:  { const float y = x + b;                                                              // Diode: soft one side, hard the other
                           return (y > 0.0f ? std::tanh (y * 1.4f) / 1.4f * 1.2f : -std::min (0.55f, -y * 0.9f)); }
                case 3:  return std::sin ((x + b) * 1.5f) - std::sin (b * 1.5f);                               // Fold
                case 4:  return std::clamp (x + b, -1.0f, 1.0f) - std::clamp (b, -1.0f, 1.0f);                 // Hard
                default: { const float y = std::tanh (x);                                                      // Rectify (octave up)
                           return y * (0.5f - b * 0.5f) + std::abs (y) * (0.5f + b * 0.5f) - (0.25f + 0.25f * b); }
            }
        }

        static float driveGain (float drive) { return std::pow (10.0f, drive * 36.0f / 20.0f); }
        static float compensation (float drive) { return 1.0f / (1.0f + drive * 2.2f); }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            dry.copyFrom (0, 0, b.l, b.n);
            dry.copyFrom (1, 0, b.r, b.n);

            const int mode = v.i (FxSatMode);
            const float bias = v[FxSatBias];
            const float gain = driveGain (v[FxSatDrive]);
            const float comp = compensation (v[FxSatDrive]);
            if (lastGain < 0.0f) lastGain = gain;

            const int os = std::clamp (v.i (FxSatOs), 0, 2);
            float* chans[] = { b.l, b.r };
            juce::dsp::AudioBlock<float> block (chans, 2, (size_t) b.n);
            auto run = [&] (juce::dsp::AudioBlock<float> blk)
            {
                const auto n = blk.getNumSamples();
                const float step = (gain - lastGain) / (float) n;
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    auto* d = blk.getChannelPointer (ch);
                    float g = lastGain;
                    for (size_t i = 0; i < n; ++i)
                    {
                        g += step;
                        d[i] = shape (mode, d[i] * g, bias) * comp;
                    }
                }
            };
            if (os == 0)
                run (block);
            else
            {
                auto& o = *oversamplers[(size_t) os - 1];
                run (o.processSamplesUp (block));
                o.processSamplesDown (block);
            }
            lastGain = gain;

            // Tilt tone around 900 Hz, output level, auto gain, dry/wet
            const float t = v[FxSatTone];
            const float lowG = 1.0f - std::max (0.0f, t) * 0.6f, highG = 1.0f + t * (t > 0.0f ? 1.0f : 0.8f);
            const float c = onePoleCoef (900.0f, sampleRate);
            const float out = juce::Decibels::decibelsToGain (v[FxSatOut]);
            const bool autoOn = v.b (FxSatAuto);
            const float rmsC = onePoleCoef (3.0f, sampleRate);
            float inPeak = 0.0f, outPeak = 0.0f;
            for (int i = 0; i < b.n; ++i)
            {
                tilt[0] += (b.l[i] - tilt[0]) * c;
                tilt[1] += (b.r[i] - tilt[1]) * c;
                float wl = (tilt[0] * lowG + (b.l[i] - tilt[0]) * highG) * out;
                float wr = (tilt[1] * lowG + (b.r[i] - tilt[1]) * highG) * out;
                const float dl = dry.getSample (0, i), dr = dry.getSample (1, i);
                if (autoOn)
                {
                    inRms += (dl * dl + dr * dr - inRms) * rmsC;
                    outRms += (wl * wl + wr * wr - outRms) * rmsC;
                    const float target = outRms > 1.0e-9f ? std::clamp (std::sqrt (inRms / outRms), 0.25f, 4.0f) : 1.0f;
                    const float g = autoGain.process (target);
                    wl *= g;
                    wr *= g;
                }
                const float m = mix.process (v[FxSatMix]);
                inPeak = std::max (inPeak, std::max (std::abs (dl), std::abs (dr)));
                b.l[i] = dl * (1.0f - m) + wl * m;
                b.r[i] = dr * (1.0f - m) + wr * m;
                outPeak = std::max (outPeak, std::max (std::abs (b.l[i]), std::abs (b.r[i])));
            }
            lastIn = inPeak;
            lastOut = outPeak;
        }

        float lastIn = 0.0f, lastOut = 0.0f;

    private:
        float sampleRate = 44100.0f, lastGain = -1.0f, inRms = 0.0f, outRms = 0.0f;
        std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> oversamplers {
            std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true),
            std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true) };
        juce::AudioBuffer<float> dry;
        std::array<float, 2> tilt {};
        Smoother mix, autoGain;
    };

    // ---------------------------------------------------------------------------------------------
    // Lo-Fi: tape wow and flutter, bit depth, sample-rate reduction with jitter, hiss and age (darker, narrower).
    // With wow, flutter, age and hiss at zero it is a clean bitcrusher.

    class Lofi
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            for (auto& d : lines) d.prepare ((int) (0.05 * sr));
            for (auto& d : dryLines) d.prepare (16);
            mix.setTime (0.02f, sampleRate);
            reset();
        }

        void reset()
        {
            for (auto& d : lines) d.clear();
            for (auto& d : dryLines) d.clear();
            hold = { 0.0f, 0.0f };
            lp = { 0.0f, 0.0f };
            hp = { 0.0f, 0.0f };
            phase = 1.0f;
            step = 1.0f;
            hissGate = 0.0f;
            rng.seed (91);
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const float wowDepth = v[FxLofiWow] * 0.004f * sampleRate, flutterDepth = v[FxLofiFlutter] * 0.0006f * sampleRate;
            const float q = std::pow (2.0f, v[FxLofiBits] - 1.0f);
            const float baseStep = 1.0f / std::max (1.0f, v[FxLofiRate]);
            const float jitter = v[FxLofiJitter];
            const float age = v[FxLofiAge];
            const bool aged = age > 1.0e-4f;
            const float lpc = onePoleCoef (16000.0f * std::pow (0.12f, age), sampleRate);
            const float hpc = onePoleCoef (20.0f + age * 180.0f, sampleRate);
            const float hiss = v[FxLofiNoise] * 0.02f;
            const float gateAtt = onePoleCoef (200.0f, sampleRate), gateRel = onePoleCoef (1.5f, sampleRate);
            float* io[] = { b.l, b.r };

            for (int i = 0; i < b.n; ++i)
            {
                wowPhase += 0.55f / sampleRate;
                flutterPhase += 9.0f / sampleRate;
                if (wowPhase > 1.0f) wowPhase -= 1.0f;
                if (flutterPhase > 1.0f) flutterPhase -= 1.0f;
                const float d = 4.0f + wowDepth * (1.0f + std::sin (kTwoPi * wowPhase)) + flutterDepth * (1.0f + std::sin (kTwoPi * flutterPhase));
                phase += step;
                const bool sampleNow = phase >= 1.0f;
                if (sampleNow)
                {
                    phase -= 1.0f;
                    step = baseStep * (1.0f + jitter * 0.9f * rng.nextBipolar());
                }
                const float m = mix.process (v[FxLofiMix]);
                // The hiss rides under the signal and fades with it (a tape that stops when the music stops)
                const float level = std::min (1.0f, std::max (std::abs (b.l[i]), std::abs (b.r[i])) * 6.0f);
                hissGate += (level - hissGate) * (level > hissGate ? gateAtt : gateRel);
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    const float in = io[ch][i];
                    lines[ch].push (in);
                    dryLines[ch].push (in);
                    const float x = dryLines[ch].read (4.0f);   // the dry path has the same 4 samples latency
                    float y = lines[ch].read (d);
                    if (sampleNow) hold[ch] = std::round (y * q) / q;
                    y = hold[ch];
                    if (aged)
                    {
                        lp[ch] += (y - lp[ch]) * lpc;
                        hp[ch] += (lp[ch] - hp[ch]) * hpc;
                        y = softClip ((lp[ch] - hp[ch]) * (1.0f + age * 0.6f));
                    }
                    y += hiss * hissGate * rng.nextBipolar();
                    io[ch][i] = x + (y - x) * m;
                }
                // Age narrows the image a little
                if (aged)
                {
                    const float mid = 0.5f * (b.l[i] + b.r[i]), side = 0.5f * (b.l[i] - b.r[i]) * (1.0f - age * 0.5f * m);
                    b.l[i] = mid + side;
                    b.r[i] = mid - side;
                }
            }
        }

        float getWow() const { return wowPhase; }

    private:
        float sampleRate = 44100.0f, phase = 1.0f, step = 1.0f, wowPhase = 0.0f, flutterPhase = 0.0f, hissGate = 0.0f;
        std::array<DelayLine, 2> lines, dryLines;
        std::array<float, 2> hold {}, lp {}, hp {};
        dsp::Random rng;
        Smoother mix;
    };

    // ---------------------------------------------------------------------------------------------
    // Transient shaper: compares fast and slow envelope followers (level independent)

    class TransientShaper
    {
    public:
        void prepare (double sr, int) { sampleRate = (float) sr; mix.setTime (0.02f, sampleRate); reset(); }
        void reset() { e.fill (0.0f); gainDb = 0.0f; mix.reset (-1.0f); }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const float speed = v[FxTransSpeed];
            auto coef = [this] (float seconds) { return std::exp (-1.0f / (std::max (0.00005f, seconds) * sampleRate)); };
            const float fastA = coef (0.0005f), slowA = coef (0.004f + (1.0f - speed) * 0.036f), rel = coef (0.12f);
            const float susLong = coef (0.04f + (1.0f - speed) * 0.3f), susShort = coef (0.006f), att = coef (0.001f);
            const float attack = v[FxTransAttack], sustain = v[FxTransSustain];
            const float out = juce::Decibels::decibelsToGain (v[FxTransOut]);
            float minG = 0.0f, maxG = 0.0f;

            auto follow = [] (float& s, float x, float a, float r) { s = x > s ? x + (s - x) * a : x + (s - x) * r; };
            if (mix.value < 0.0f) mix.reset (v[FxTransMix]);   // new smoothers start at their value (no fade on the first hit)

            for (int i = 0; i < b.n; ++i)
            {
                const float x = std::max (std::abs (b.l[i]), std::abs (b.r[i]));
                follow (e[0], x, fastA, rel);
                follow (e[1], x, slowA, rel);
                follow (e[2], x, att, susLong);
                follow (e[3], x, att, susShort);
                const float dA = std::max (0.0f, e[0] - e[1]) / std::max (1.0e-5f, e[0]);
                const float dS = std::max (0.0f, e[2] - e[3]) / std::max (1.0e-5f, e[2]);
                const float target = attack * 18.0f * dA + sustain * 18.0f * dS;
                gainDb += (target - gainDb) * 0.2f;
                const float m = mix.process (v[FxTransMix]);
                const float g = 1.0f - m + juce::Decibels::decibelsToGain (gainDb) * out * m;
                b.l[i] *= g;
                b.r[i] *= g;
                minG = std::min (minG, gainDb);
                maxG = std::max (maxG, gainDb);
            }
            lastGainDb = std::abs (maxG) > std::abs (minG) ? maxG : minG;
        }

        float lastGainDb = 0.0f;

    private:
        float sampleRate = 44100.0f, gainDb = 0.0f;
        std::array<float, 4> e {};
        Smoother mix;
    };

    // ---------------------------------------------------------------------------------------------
    // Compressor: stereo linked, soft knee, parallel mix, sidechain high-pass, automatic makeup

    class Compressor
    {
    public:
        void prepare (double sr, int) { sampleRate = (float) sr; mix.setTime (0.02f, sampleRate); reset(); }
        void reset() { envDb = 0.0f; hpX = { 0.0f, 0.0f }; hpY = { 0.0f, 0.0f }; }

        // Static curve: gain reduction in dB for an input level (shared with the graphs)
        static float reduction (float levelDb, float threshold, float ratio, float knee)
        {
            const float over = levelDb - threshold;
            const float slope = 1.0f - 1.0f / std::max (1.0f, ratio);
            if (knee < 0.01f || over >= knee * 0.5f) return std::max (0.0f, over) * slope;
            if (over <= -knee * 0.5f) return 0.0f;
            const float k = over + knee * 0.5f;
            return slope * k * k / (2.0f * knee);
        }

        // Automatic makeup: gives back half of the reduction a full-scale signal would get
        static float autoMakeup (float threshold, float ratio) { return 0.5f * reduction (0.0f, threshold, ratio, 0.0f); }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const float att = std::exp (-1.0f / (std::max (0.0001f, v[FxCompAttack] * 0.001f) * sampleRate));
            const float rel = std::exp (-1.0f / (std::max (0.005f, v[FxCompRelease] * 0.001f) * sampleRate));
            const float threshold = v[FxCompThresh], ratio = v[FxCompRatio], knee = v[FxCompKnee];
            const float makeup = juce::Decibels::decibelsToGain (v[FxCompMakeup] + (v.b (FxCompAuto) ? autoMakeup (threshold, ratio) : 0.0f));
            const float hpfHz = v[FxCompHpf];
            const bool hpf = hpfHz > 20.5f;
            const float hpA = 1.0f / (1.0f + kTwoPi * hpfHz / sampleRate);
            float maxRed = 0.0f, inPeak = 0.0f, outPeak = 0.0f;

            for (int i = 0; i < b.n; ++i)
            {
                float dl = b.l[i], dr = b.r[i];
                inPeak = std::max (inPeak, std::max (std::abs (dl), std::abs (dr)));
                if (hpf)
                {
                    // the detector ignores the lows, so kicks and basses do not pump the whole mix
                    hpY[0] = hpA * (hpY[0] + dl - hpX[0]); hpX[0] = dl; dl = hpY[0];
                    hpY[1] = hpA * (hpY[1] + dr - hpX[1]); hpX[1] = dr; dr = hpY[1];
                }
                const float level = juce::Decibels::gainToDecibels (std::max (std::abs (dl), std::abs (dr)), -120.0f);
                const float red = reduction (level, threshold, ratio, knee);
                envDb = red > envDb ? red + (envDb - red) * att : red + (envDb - red) * rel;
                maxRed = std::max (maxRed, envDb);
                const float g = juce::Decibels::decibelsToGain (-envDb) * makeup;
                const float m = mix.process (v[FxCompMix]);
                b.l[i] = b.l[i] * (1.0f - m) + b.l[i] * g * m;
                b.r[i] = b.r[i] * (1.0f - m) + b.r[i] * g * m;
                outPeak = std::max (outPeak, std::max (std::abs (b.l[i]), std::abs (b.r[i])));
            }
            lastReduction = maxRed;
            lastIn = inPeak;
            lastOut = outPeak;
        }

        float lastReduction = 0.0f, lastIn = 0.0f, lastOut = 0.0f;

    private:
        float sampleRate = 44100.0f, envDb = 0.0f;
        std::array<float, 2> hpX {}, hpY {};
        Smoother mix;
    };

    // ---------------------------------------------------------------------------------------------
    // Squash: three-band upward + downward compression ("OTT" style) – makes quiet details loud.

    class Squash
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = sr;
            for (auto& f : lowA) f.lowpass (sr, 120.0, 0.7071);
            for (auto& f : lowB) f.lowpass (sr, 120.0, 0.7071);
            for (auto& f : highA) f.highpass (sr, 2500.0, 0.7071);
            for (auto& f : highB) f.highpass (sr, 2500.0, 0.7071);
            mix.setTime (0.02f, (float) sr);
            reset();
        }

        void reset()
        {
            for (auto& f : lowA) f.reset();
            for (auto& f : lowB) f.reset();
            for (auto& f : highA) f.reset();
            for (auto& f : highB) f.reset();
            env.fill (0.0f);
            mix.reset (-1.0f);
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            if (mix.value < 0.0f) mix.reset (v[FxSqMix]);
            const float depth = v[FxSqDepth], up = v[FxSqUpward];
            const float t = 0.2f + v[FxSqTime] * 2.0f;
            const float att = std::exp (-1.0f / (0.003f * t * (float) sampleRate));
            const float rel = std::exp (-1.0f / (0.09f * t * (float) sampleRate));
            const float out = juce::Decibels::decibelsToGain (v[FxSqOut]);
            float maxGr = 0.0f;
            for (int i = 0; i < b.n; ++i)
            {
                float bands[3][2];
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    const float x = ch == 0 ? b.l[i] : b.r[i];
                    const float lo = lowB[ch].process (lowA[ch].process (x, 0), 0);
                    const float hi = highB[ch].process (highA[ch].process (x, 0), 0);
                    bands[0][ch] = lo;
                    bands[2][ch] = hi;
                    bands[1][ch] = x - lo - hi;
                }
                float yl = 0.0f, yr = 0.0f;
                for (size_t k = 0; k < 3; ++k)
                {
                    const float level = std::max (std::abs (bands[k][0]), std::abs (bands[k][1]));
                    env[k] = level > env[k] ? level + (env[k] - level) * att : level + (env[k] - level) * rel;
                    const float db = juce::Decibels::gainToDecibels (env[k] + 1.0e-6f, -100.0f);
                    // Downward above -24 dB (4:1), upward below -36 dB (up to +18 dB)
                    float gainDb = 0.0f;
                    if (db > -24.0f) gainDb -= (db + 24.0f) * 0.75f * depth;
                    if (db < -36.0f && db > -80.0f) gainDb += std::min (18.0f, (-36.0f - db) * 0.6f) * depth * up;
                    maxGr = std::max (maxGr, std::abs (gainDb));
                    const float g = juce::Decibels::decibelsToGain (gainDb) * (k == 1 ? 1.0f : 1.1f);
                    yl += bands[k][0] * g;
                    yr += bands[k][1] * g;
                }
                const float m = mix.process (v[FxSqMix]);
                b.l[i] = b.l[i] * (1.0f - m) + yl * out * m;
                b.r[i] = b.r[i] * (1.0f - m) + yr * out * m;
            }
            lastActivity = maxGr;
        }

        float lastActivity = 0.0f;

    private:
        double sampleRate = 44100.0;
        std::array<Biquad, 2> lowA, lowB, highA, highB;
        std::array<float, 3> env {};
        Smoother mix;
    };

    // ---------------------------------------------------------------------------------------------
    // EQ: low cut (12 / 24 dB), low shelf, two bells, high shelf, high cut (12 / 24 dB).

    struct EqDesign
    {
        static constexpr int kNumFilters = 8;
        std::array<Biquad, kNumFilters> filters;
        std::array<bool, kNumFilters> use {};
        std::array<int, kNumFilters> band {};   // 0 low cut, 1 low, 2 mid, 3 mid 2, 4 high, 5 high cut

        void design (double sr, const Values& v)
        {
            const double safe = sr * 0.45;
            constexpr double q12 = 0.7071, q24a = 0.5412, q24b = 1.3066;
            auto f = [safe] (float hz) { return std::min ((double) hz, safe); };
            const bool lc24 = v.i (FxEqLowSlope) == 1, hc24 = v.i (FxEqHighSlope) == 1;
            filters[0].highpass (sr, f (v[FxEqLowCut]), lc24 ? q24a : q12);
            filters[1].highpass (sr, f (v[FxEqLowCut]), q24b);
            filters[2].shelf (sr, f (v[FxEqLowFreq]), v[FxEqLowGain], false);
            filters[3].peak (sr, f (v[FxEqMidFreq]), v[FxEqMidQ], v[FxEqMidGain]);
            filters[4].peak (sr, f (v[FxEqMid2Freq]), v[FxEqMid2Q], v[FxEqMid2Gain]);
            filters[5].shelf (sr, f (v[FxEqHighFreq]), v[FxEqHighGain], true);
            filters[6].lowpass (sr, f (v[FxEqHighCut]), hc24 ? q24a : q12);
            filters[7].lowpass (sr, f (v[FxEqHighCut]), q24b);
            const bool lowCut = v[FxEqLowCut] > 20.5f, highCut = v[FxEqHighCut] < 19900.0f;
            use = { lowCut, lowCut && lc24, std::abs (v[FxEqLowGain]) > 0.01f, std::abs (v[FxEqMidGain]) > 0.01f,
                    std::abs (v[FxEqMid2Gain]) > 0.01f, std::abs (v[FxEqHighGain]) > 0.01f, highCut, highCut && hc24 };
            band = { 0, 0, 1, 2, 3, 4, 5, 5 };
        }

        double magnitudeDb (double freq, double sr, int onlyBand = -1) const
        {
            double db = 0.0;
            for (size_t k = 0; k < filters.size(); ++k)
                if (use[k] && (onlyBand < 0 || band[k] == onlyBand))
                    db += 20.0 * std::log10 (std::max (1.0e-9, filters[k].magnitude (freq, sr)));
            return db;
        }
    };

    class Eq
    {
    public:
        void prepare (double sr, int) { sampleRate = sr; reset(); last.fill (-1.0e9f); }
        void reset() { for (auto& f : eq.filters) f.reset(); }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            // Redesign only when a value really changed
            bool changed = false;
            for (int p = FxEqLowCut; p <= FxEqHighSlope; ++p)
                if (! juce::exactlyEqual (last[(size_t) (p - FxEqLowCut)], v[(Param) p]))
                {
                    last[(size_t) (p - FxEqLowCut)] = v[(Param) p];
                    changed = true;
                }
            if (changed) eq.design (sampleRate, v);

            for (int i = 0; i < b.n; ++i)
            {
                float l = b.l[i], r = b.r[i];
                for (size_t k = 0; k < eq.filters.size(); ++k)
                    if (eq.use[k]) { l = eq.filters[k].process (l, 0); r = eq.filters[k].process (r, 1); }
                b.l[i] = l;
                b.r[i] = r;
            }
        }

    private:
        double sampleRate = 44100.0;
        EqDesign eq;
        std::array<float, FxEqHighSlope - FxEqLowCut + 1> last {};
    };

    // ---------------------------------------------------------------------------------------------
    // Filter: ladder (4-pole, saturating) or SVF, an envelope that restarts on every note / hit (or on onsets in the
    // audio) and an LFO.

    class Filter
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            mix.setTime (0.02f, sampleRate);
            cutoff.setTime (0.01f, sampleRate);
            reset();
        }

        void reset()
        {
            for (auto& s : ladder) s.fill (0.0f);
            for (auto& s : svf) s.reset();
            env = 0.0f;
            cutoff.reset (-1.0f);
            fast = slow = 0.0f;
            holdoff = 0;
            rnd.seed (5);
            randomValue = randomTarget = 0.0f;
        }

        // LFO shapes 0..1 -> -1..1 (shared with the graph)
        static float lfoShape (int shape, float ph, float random)
        {
            switch (shape)
            {
                case 1:  return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
                case 2:  return 2.0f * ph - 1.0f;
                case 3:  return ph < 0.5f ? 1.0f : -1.0f;
                case 4:  return random;
                default: return std::sin (kTwoPi * ph);
            }
        }

        void process (StereoBuffer b, const Values& v, const Context& c)
        {
            const int type = v.i (FxFiltType);
            const float reso = v[FxFiltReso], amount = v[FxFiltEnv];
            const float decay = std::exp (-1.0f / (std::max (0.002f, v[FxFiltDecay]) * sampleRate));
            const float drive = 1.0f + v[FxFiltDrive] * 6.0f;
            const bool audioTrigger = v.i (FxFiltTrigger) == 1 || c.triggers == nullptr;
            const float lfoDepth = v[FxFiltLfoDepth];
            const int shape = v.i (FxFiltLfoShape);
            const double inc = lfo.begin (v.b (FxFiltLfoSync), v[FxFiltLfoRate], v.i (FxFiltLfoNote), c, sampleRate);
            const float fastC = onePoleCoef (300.0f, sampleRate), slowC = onePoleCoef (8.0f, sampleRate);
            if (cutoff.value < 0.0f) cutoff.reset (v[FxFiltCutoff]);
            int h = 0;
            float* io[] = { b.l, b.r };

            for (int i = 0; i < b.n; ++i)
            {
                if (! audioTrigger)
                    while (h < c.numTriggers && c.triggers[h] <= i) { env = 1.0f; ++h; }
                else
                {
                    // Onsets: the fast level jumps well above the slow one
                    const float x = std::max (std::abs (b.l[i]), std::abs (b.r[i]));
                    fast += (x - fast) * fastC;
                    slow += (x - slow) * slowC;
                    if (holdoff > 0) --holdoff;
                    else if (fast > 0.01f && fast > slow * 2.2f) { env = 1.0f; holdoff = (int) (0.06f * sampleRate); }
                }
                const float before = (float) lfo.phase;
                const float ph = lfo.advance (inc);
                if (ph < before)
                {
                    randomTarget = rnd.nextBipolar();
                }
                randomValue += (randomTarget - randomValue) * 0.002f;
                const float mod = lfoDepth > 0.0f ? lfoDepth * lfoShape (shape, ph, randomValue) : 0.0f;
                const float fc = std::clamp (cutoff.process (v[FxFiltCutoff]) * std::exp2 (amount * env + mod), 20.0f, sampleRate * 0.45f);
                env *= decay;
                lastCutoff = fc;
                const float m = mix.process (v[FxFiltMix]);
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float x = io[ch][i];
                    float y;
                    if (type == 0)
                        y = processLadder (ch, x * drive, fc, reso) / std::sqrt (drive);
                    else
                    {
                        const int svfType = type == 1 ? dsp::Svf::Low : type == 2 ? dsp::Svf::Band : type == 3 ? dsp::Svf::High : dsp::Svf::Notch;
                        y = svf[(size_t) ch].process (softClip (x * drive) / std::sqrt (drive), fc, reso, sampleRate, svfType);
                    }
                    io[ch][i] = x * (1.0f - m) + y * m;
                }
            }
        }

        float currentEnv() const { return env; }
        float lastCutoff = 1000.0f;

    private:
        float processLadder (int ch, float x, float fc, float reso)
        {
            auto& s = ladder[(size_t) ch];
            const float g = 1.0f - std::exp (-kTwoPi * fc / sampleRate);
            const float k = reso * 3.9f;
            float in = std::tanh (x - k * s[3]);
            for (int i = 0; i < 4; ++i)
            {
                s[(size_t) i] += g * (std::tanh (in) - std::tanh (s[(size_t) i]));
                in = s[(size_t) i];
            }
            return s[3] * (1.0f + k * 0.35f);
        }

        float sampleRate = 44100.0f, env = 0.0f, fast = 0.0f, slow = 0.0f, randomValue = 0.0f, randomTarget = 0.0f;
        int holdoff = 0;
        LfoPhase lfo;
        dsp::Random rnd;
        std::array<std::array<float, 4>, 2> ladder {};
        std::array<dsp::Svf, 2> svf;
        Smoother mix, cutoff;
    };

    // ---------------------------------------------------------------------------------------------
    // Motion: chorus (two modulated taps, the right channel in anti-phase – wide and soft), ensemble (three voices),
    // phaser (6 allpass stages), flanger, doubler (a second, late and detuned take) or vibrato (pitch only)

    class Motion
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            for (auto& d : lines) d.prepare ((int) (0.06 * sr));
            mix.setTime (0.02f, sampleRate);
            depth.setTime (0.05f, sampleRate);
            reset();
        }

        void reset()
        {
            for (auto& d : lines) d.clear();
            for (auto& s : allpass) s.fill (0.0f);
            last = { 0.0f, 0.0f };
        }

        void process (StereoBuffer b, const Values& v, const Context& c)
        {
            const int mode = v.i (FxMotMode);
            const double inc = lfo.begin (v.b (FxMotSync), v[FxMotRate], v.i (FxMotNote), c, sampleRate);
            const float fb = v[FxMotFeedback];
            float* io[] = { b.l, b.r };
            auto tri = [] (float ph) { ph -= std::floor (ph); return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph; };

            for (int i = 0; i < b.n; ++i)
            {
                const float dep = depth.process (v[FxMotDepth]);
                const float m = mix.process (v[FxMotMix]);
                const float phase = (float) lfo.phase;
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float ph = phase + (ch == 0 ? 0.0f : 0.25f);
                    const float lfoValue = 0.5f - 0.5f * std::cos (kTwoPi * ph);
                    const float x = io[ch][i];
                    auto& line = lines[(size_t) ch];
                    float y = 0.0f, out = 0.0f;
                    switch (mode)
                    {
                        case 0:     // Chorus
                        {
                            line.push (x + last[(size_t) ch] * fb * 0.4f);
                            const float base = 0.0075f * sampleRate, d = dep * 0.0055f * sampleRate;
                            const float a = tri (phase), cc = tri (phase + 0.33f);
                            y = ch == 0 ? 0.5f * (line.read (base + d * a) + line.read (base * 1.3f + d * cc))
                                        : 0.5f * (line.read (base - d * a + d) + line.read (base * 1.3f - d * cc + d));
                            out = x * (1.0f - 0.5f * m) + y * m * 0.8f;
                            break;
                        }
                        case 1:     // Ensemble: three taps, 120 degrees apart
                        {
                            line.push (x);
                            for (int t = 0; t < 3; ++t)
                            {
                                const float p = ph + (float) t / 3.0f;
                                const float l2 = 0.5f - 0.5f * std::cos (kTwoPi * p);
                                const float fastWobble = 0.08f * std::sin (kTwoPi * (p * 7.0f));
                                y += line.read ((0.012f + dep * 0.01f * (l2 + fastWobble)) * sampleRate);
                            }
                            y *= 0.55f;
                            out = x * (1.0f - 0.5f * m) + y * 0.5f * m;
                            break;
                        }
                        case 2:     // Phaser: 6 allpass stages with feedback
                        {
                            const float fc = std::min (180.0f * std::exp2 (lfoValue * dep * 4.5f), sampleRate * 0.4f);
                            const float t = std::tan (kPi * fc / sampleRate);
                            const float coef = (t - 1.0f) / (t + 1.0f);
                            y = x + last[(size_t) ch] * fb * 0.85f;
                            for (auto& s : allpass[(size_t) ch])
                            {
                                const float o = coef * y + s;
                                s = y - coef * o;
                                y = o;
                            }
                            out = x * (1.0f - 0.5f * m) + y * 0.5f * m;
                            break;
                        }
                        case 3:     // Flanger
                            line.push (softClip (x + last[(size_t) ch] * fb * 0.9f));
                            y = line.read ((0.0004f + dep * 0.0055f * lfoValue) * sampleRate + 1.0f);
                            out = x * (1.0f - 0.5f * m) + y * 0.5f * m;
                            break;
                        case 4:     // Doubler
                        {
                            line.push (x);
                            const float base = ch == 0 ? 0.019f : 0.027f;
                            y = line.read ((base + dep * 0.0025f * lfoValue) * sampleRate);
                            out = x * (1.0f - 0.5f * m) + y * 0.8f * m;
                            break;
                        }
                        default:    // Vibrato: only the modulated tap (pitch wobble), mix blends it in
                            line.push (x);
                            y = line.read ((0.005f + dep * 0.004f * lfoValue) * sampleRate);
                            out = x * (1.0f - m) + y * m;
                            break;
                    }
                    last[(size_t) ch] = softClip (y);
                    io[ch][i] = out;
                }
                lfo.advance (inc);
            }
        }

        float getPhase() const { return (float) lfo.phase; }

    private:
        float sampleRate = 44100.0f;
        LfoPhase lfo;
        std::array<DelayLine, 2> lines;
        std::array<std::array<float, 6>, 2> allpass {};
        std::array<float, 2> last {};
        Smoother mix, depth;
    };

    // Schroeder allpass (diffusion in the reverb and the diffuse delay)
    struct Allpass
    {
        DelayLine line;
        int length = 1;
        void prepare (int len) { line.prepare (len + 4); length = len; }
        void clear() { line.clear(); }
        float process (float x, float g)
        {
            const float d = line.read ((float) length);
            const float v = x + g * d;
            line.push (v);
            return d - g * v;
        }
    };

    // ---------------------------------------------------------------------------------------------
    // Delay: clean, tape (wow, saturation, darker repeats), grains (repeats scattered an octave up / a fifth down) or
    // diffuse (repeats smear into a cloud). Tempo synced, ping-pong, stereo offset, modulation, ducking.

    class Delay
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            for (auto& d : lines) d.prepare ((int) (4.1 * sr));
            for (auto& p : shifters) p.prepare (sr, 90.0f);
            const float scale = sampleRate / 44100.0f;
            static constexpr float lengths[2][4] = { { 223, 341, 467, 593 }, { 239, 353, 479, 607 } };
            for (size_t c = 0; c < 2; ++c)
                for (size_t k = 0; k < 4; ++k)
                    diffusers[c][k].prepare ((int) (lengths[c][k] * scale) + 2);
            mix.setTime (0.03f, sampleRate);
            time.setTime (0.08f, sampleRate);   // time changes glide like a tape echo
            reset();
        }

        void reset()
        {
            for (auto& d : lines) d.clear();
            for (auto& p : shifters) p.clear();
            for (auto& c : diffusers) for (auto& d : c) d.clear();
            lp.fill (0.0f);
            hp.fill (0.0f);
            duckEnv = 0.0f;
            time.reset (-1.0f);
            wow = 0.0f;
            grainFlip = 0;
            grainCounter = 0;
        }

        static float delaySeconds (const Values& v, double bpm)
        {
            return v.b (FxDlySync) ? (float) (noteBeats (v.i (FxDlyNote)) * 60.0 / std::max (20.0, bpm)) : v[FxDlyTime] * 0.001f;
        }

        void process (StereoBuffer b, const Values& v, const Context& c)
        {
            const float target = std::clamp (delaySeconds (v, c.bpm) * sampleRate, 16.0f, (float) lines[0].maxDelay() / 1.3f - 64.0f);
            if (time.value < 0.0f) time.reset (target);

            const int mode = v.i (FxDlyMode);
            const float lpc = onePoleCoef (18000.0f * std::pow (0.07f, std::min (1.0f, v[FxDlyDamp] + (mode == 1 ? 0.25f : 0.0f))), sampleRate);
            const float hpc = onePoleCoef (v[FxDlyLowCut], sampleRate);
            const float duckAtt = 1.0f - std::exp (-1.0f / (0.005f * sampleRate));
            const float duckRel = 1.0f - std::exp (-1.0f / (0.3f * sampleRate));
            const float fb = v[FxDlyFeedback], duckAmt = v[FxDlyDuck];
            const float offset = 1.0f + v[FxDlySpread] * 0.3f;
            const float modDepth = v[FxDlyMod] * 0.003f * sampleRate;
            const bool pingPong = v.b (FxDlyPingPong);

            for (int i = 0; i < b.n; ++i)
            {
                float d = time.process (target);
                wow += (mode == 1 ? 0.6f : 0.35f) / sampleRate;
                if (wow > 1.0f) wow -= 1.0f;
                if (mode == 1)
                    d += (0.0018f * std::sin (kTwoPi * wow) + 0.0004f * std::sin (kTwoPi * wow * 7.3f)) * sampleRate;
                d += modDepth * std::sin (kTwoPi * wow);
                float wl = lines[0].read (d), wr = lines[1].read (d * offset);
                if (mode == 2)
                {
                    if (++grainCounter > (int) (sampleRate * 0.09f)) { grainCounter = 0; grainFlip = (grainFlip + 1) % 3; }
                    const float ratio = grainFlip == 0 ? 2.0f : (grainFlip == 1 ? 1.0f : 0.667f);
                    wl = shifters[0].process (wl, ratio);
                    wr = shifters[1].process (wr, ratio);
                }
                else if (mode == 3)
                    for (size_t k = 0; k < 4; ++k)
                    {
                        wl = diffusers[0][k].process (wl, 0.6f);
                        wr = diffusers[1][k].process (wr, 0.6f);
                    }

                auto colour = [&] (float x, int ch)
                {
                    lp[(size_t) ch] += (x - lp[(size_t) ch]) * lpc;
                    hp[(size_t) ch] += (lp[(size_t) ch] - hp[(size_t) ch]) * hpc;
                    const float y = lp[(size_t) ch] - hp[(size_t) ch];
                    return mode == 1 ? softClip (y * 1.4f) / 1.2f : softClip (y);
                };
                const float cl = colour (wl, 0), cr = colour (wr, 1);

                const float inL = b.l[i], inR = b.r[i];
                if (pingPong)
                {
                    lines[0].push ((inL + inR) * 0.5f + cr * fb);
                    lines[1].push (cl * fb);
                }
                else
                {
                    lines[0].push (inL + cl * fb);
                    lines[1].push (inR + cr * fb);
                }

                const float level = std::max (std::abs (inL), std::abs (inR));
                duckEnv += (level - duckEnv) * (level > duckEnv ? duckAtt : duckRel);
                const float duck = 1.0f - duckAmt * std::min (1.0f, duckEnv * 4.0f);

                // Clean repeats come out as they are (the colouring happens in the feedback), the others coloured
                const float outL = mode == 0 ? wl : cl, outR = mode == 0 ? wr : cr;
                float dg, wg;
                equalPowerMix (mix.process (v[FxDlyMix]), dg, wg);
                b.l[i] = inL * dg + outL * wg * duck;
                b.r[i] = inR * dg + outR * wg * duck;
            }
        }

    private:
        float sampleRate = 44100.0f, duckEnv = 0.0f, wow = 0.0f;
        int grainCounter = 0, grainFlip = 0;
        std::array<DelayLine, 2> lines;
        std::array<dsp::PitchShifter, 2> shifters;
        std::array<std::array<Allpass, 4>, 2> diffusers;
        std::array<float, 2> lp {}, hp {};
        Smoother mix, time;
    };

    // ---------------------------------------------------------------------------------------------
    // Reverb: 8-line feedback delay network (Hadamard), input diffusion, slowly modulated lines.
    // Voicings: room / plate / hall, shimmer (octave up in the feedback), cloud (long, heavily modulated) and studio
    // (Fjord's original voicing). Input low cut, ducking, freeze.

    class Reverb
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            const float scale = sampleRate / 44100.0f;
            for (size_t i = 0; i < 8; ++i)
                lines[i].prepare ((int) (baseLengths[i] * 3.6f * scale) + 64);
            for (size_t c = 0; c < 2; ++c)
                for (size_t k = 0; k < 4; ++k)
                    diffusers[c][k].prepare ((int) (diffLengths[c][k] * 1.4f * scale) + 4);
            predelay.prepare ((int) (0.3 * sr));
            for (auto& p : shifters) p.prepare (sr, 80.0f);
            mix.setTime (0.03f, sampleRate);
            size.setTime (0.25f, sampleRate);
            size.reset (-1.0f);
            reset();
        }

        void reset()
        {
            for (auto& l : lines) l.clear();
            for (auto& c : diffusers) for (auto& d : c) d.clear();
            for (auto& p : shifters) p.clear();
            predelay.clear();
            damp.fill (0.0f);
            hp = { 0.0f, 0.0f };
            duckEnv = 0.0f;
            shimmerLp = { 0.0f, 0.0f };
            tailLevel = 0.0f;
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const float scale = sampleRate / 44100.0f;
            const int mode = v.i (FxVerbMode);
            const bool studio = mode == 5;
            const float modeLen = mode == 0 ? 0.55f : mode == 1 ? 0.85f : mode == 4 ? 1.7f : 1.35f;
            const float modeDamp = mode == 1 ? 0.5f : 1.0f;            // plates are brighter
            const float diffusion = mode == 1 ? 0.72f : mode == 4 ? 0.78f : 0.62f;
            const float modScale = v[FxVerbMod] / 0.3f;
            const float modDepth = (studio ? 7.0f : mode == 4 ? 28.0f : (mode == 3 ? 9.0f : 5.0f)) * modScale;
            const float shimmer = mode == 3 ? v[FxVerbShimmer] : (mode == 4 ? v[FxVerbShimmer] * 0.25f : 0.0f);
            const bool freeze = v.b (FxVerbFreeze);
            if (size.value < 0.0f) size.reset (v[FxVerbSize]);
            const float dampC = freeze ? 1.0f : onePoleCoef (16000.0f * std::pow (0.08f, v[FxVerbDamp] * modeDamp), sampleRate);
            const float preSamples = std::max (1.0f, v[FxVerbPre] * sampleRate);
            const float t60 = std::max (0.1f, v[FxVerbDecay]);
            const float hpC = onePoleCoef (v[FxVerbLowCut], sampleRate);
            const bool lowCut = v[FxVerbLowCut] > 20.5f;
            const float duckAmt = v[FxVerbDuck], width = v[FxVerbWidth];
            const float duckAtt = 1.0f - std::exp (-1.0f / (0.002f * sampleRate));
            const float duckRel = 1.0f - std::exp (-1.0f / (0.25f * sampleRate));
            const float shimLp = onePoleCoef (6000.0f, sampleRate);
            auto lengthScale = [&] (float sz) { return studio ? (0.45f + 2.2f * sz) * scale : (0.3f + 2.0f * sz) * modeLen * scale; };

            // Input diffusion: the studio voicing keeps its original, shorter allpasses
            for (size_t c = 0; c < 2; ++c)
                for (size_t k = 0; k < 4; ++k)
                    diffusers[c][k].length = (int) (diffLengths[c][k] * (studio ? 1.0f : 1.4f) * scale) + 4;

            std::array<float, 8> decayGain {};
            for (size_t k = 0; k < 8; ++k)
                decayGain[k] = freeze ? 1.0f : std::pow (10.0f, -3.0f * baseLengths[k] * lengthScale (size.value) / (t60 * sampleRate));

            for (int i = 0; i < b.n; ++i)
            {
                const float lenScale = lengthScale (size.process (v[FxVerbSize]));

                float in = freeze ? 0.0f : (b.l[i] + b.r[i]) * 0.5f;
                if (lowCut)
                {
                    hp[0] += (in - hp[0]) * hpC;
                    in -= hp[0];
                }
                predelay.push (in);
                const float pre = predelay.read (preSamples);
                float x[2] = { pre, pre };
                for (size_t c = 0; c < 2; ++c)
                    for (size_t k = 0; k < 4; ++k)
                        x[c] = diffusers[c][k].process (x[c], diffusion);

                std::array<float, 8> y {};
                for (size_t k = 0; k < 8; ++k)
                {
                    const float len = baseLengths[k] * lenScale;
                    const float mod = modDepth * scale * std::sin (modPhase[k]);
                    modPhase[k] += modRates[k] * (mode == 4 ? 2.2f : 1.0f) * kTwoPi / sampleRate;
                    if (modPhase[k] > kTwoPi) modPhase[k] -= kTwoPi;
                    const float out = lines[k].read (std::max (4.0f, len + mod));
                    damp[k] += (out - damp[k]) * dampC;
                    y[k] = damp[k] * decayGain[k];
                }

                // Shimmer: an octave up fed back into two of the lines
                float shim[2] = { 0.0f, 0.0f };
                if (shimmer > 0.001f)
                    for (size_t c = 0; c < 2; ++c)
                    {
                        const float up = shifters[c].process (y[c * 4], 2.0f);
                        shimmerLp[c] += (up - shimmerLp[c]) * shimLp;
                        shim[c] = shimmerLp[c] * shimmer * 0.55f;
                    }

                std::array<float, 8> h = y;
                for (int len = 1; len < 8; len <<= 1)
                    for (int s = 0; s < 8; s += len * 2)
                        for (int j = s; j < s + len; ++j)
                        {
                            const float a = h[(size_t) j], c = h[(size_t) (j + len)];
                            h[(size_t) j] = a + c;
                            h[(size_t) (j + len)] = a - c;
                        }

                constexpr float norm = 0.35355339f;
                for (size_t k = 0; k < 8; ++k)
                {
                    const float fed = h[k] * norm + x[k % 2] * 0.5f + (k == 1 ? shim[0] : (k == 6 ? shim[1] : 0.0f));
                    lines[k].push (studio && ! freeze ? fed : softClip (fed));
                }

                const float outL = (y[0] + y[2] + y[4] + y[6] - y[1] * 0.5f) * 0.45f;
                const float outR = (y[1] + y[3] + y[5] + y[7] - y[0] * 0.5f) * 0.45f;
                const float mid = 0.5f * (outL + outR), side = 0.5f * (outL - outR) * width;

                const float level = std::max (std::abs (b.l[i]), std::abs (b.r[i]));
                duckEnv += (level - duckEnv) * (level > duckEnv ? duckAtt : duckRel);
                const float duck = 1.0f - duckAmt * std::min (1.0f, duckEnv * 3.0f);

                float dg, wg;
                equalPowerMix (mix.process (v[FxVerbMix]), dg, wg);
                b.l[i] = b.l[i] * dg + (mid + side) * wg * duck;
                b.r[i] = b.r[i] * dg + (mid - side) * wg * duck;
                tailLevel = tailLevel * 0.9995f + std::abs (mid) * 0.0005f;
            }
        }

        float tailLevel = 0.0f;   // for the visual

    private:
        static constexpr float baseLengths[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static constexpr float diffLengths[2][4] = { { 142, 107, 379, 277 }, { 151, 113, 367, 283 } };
        static constexpr float modRates[8] = { 0.11f, 0.17f, 0.23f, 0.29f, 0.31f, 0.37f, 0.41f, 0.47f };

        float sampleRate = 44100.0f, duckEnv = 0.0f;
        std::array<DelayLine, 8> lines;
        std::array<std::array<Allpass, 4>, 2> diffusers;
        std::array<dsp::PitchShifter, 2> shifters;
        DelayLine predelay;
        std::array<float, 8> damp {}, modPhase { 0.0f, 0.8f, 1.6f, 2.4f, 3.2f, 4.0f, 4.8f, 5.6f };
        std::array<float, 2> hp {}, shimmerLp {};
        Smoother mix, size;
    };

    // ---------------------------------------------------------------------------------------------
    // Width: mid/side, everything below the crossover kept mono, Haas spread (the right side a few ms late)

    class Width
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = sr;
            width.setTime (0.03f, (float) sr);
            width.reset (1.0f);
            haas.setTime (0.05f, (float) sr);
            haas.reset (0.0f);
            line.prepare ((int) (0.04 * sr));
            reset();
            lastFreq = -1.0f;
        }

        void reset() { for (auto& f : hp) f.reset(); line.clear(); }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            if (! juce::exactlyEqual (v[FxWidthMono], lastFreq))
            {
                lastFreq = v[FxWidthMono];
                hp[0].highpass (sampleRate, lastFreq, 0.5412);
                hp[1].highpass (sampleRate, lastFreq, 1.3066);
            }
            const bool monoLows = lastFreq > 20.5f;
            for (int i = 0; i < b.n; ++i)
            {
                const float w = width.process (v[FxWidthAmount]);
                const float mid = 0.5f * (b.l[i] + b.r[i]);
                float side = 0.5f * (b.l[i] - b.r[i]);
                if (monoLows) side = hp[1].process (hp[0].process (side, 0), 0);
                side *= w;
                b.l[i] = mid + side;
                b.r[i] = mid - side;

                const float d = haas.process (v[FxWidthHaas]) * 0.001f * (float) sampleRate;
                line.push (b.r[i]);
                if (d > 0.5f) b.r[i] = line.read (d);
            }
        }

    private:
        double sampleRate = 44100.0;
        float lastFreq = -1.0f;
        std::array<Biquad, 2> hp;
        DelayLine line;
        Smoother width, haas;
    };

    // ---------------------------------------------------------------------------------------------
    // Tremolo: volume (tremolo), position (auto-pan) or lows against highs (harmonic tremolo), tempo synced.

    class Tremolo
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            for (auto& f : low) f.lowpass (sr, 750.0, 0.7071);
            for (auto& f : high) f.highpass (sr, 750.0, 0.7071);
            mix.setTime (0.02f, sampleRate);
            depth.setTime (0.02f, sampleRate);
            reset();
        }

        void reset()
        {
            smooth = { 0.5f, 0.5f };
            for (auto& f : low) f.reset();
            for (auto& f : high) f.reset();
            rnd.seed (17);
            randoms = { 0.5f, 0.5f };
            mix.reset (-1.0f);
        }

        // 0..1 shape (shared with the graph)
        static float shape (int s, float ph, float random)
        {
            ph -= std::floor (ph);
            switch (s)
            {
                case 1:  return ph < 0.5f ? 2.0f * ph : 2.0f - 2.0f * ph;
                case 2:  return ph < 0.5f ? 1.0f : 0.0f;
                case 3:  return 1.0f - ph;
                case 4:  return random;
                default: return 0.5f + 0.5f * std::cos (kTwoPi * ph);
            }
        }

        void process (StereoBuffer b, const Values& v, const Context& c)
        {
            if (mix.value < 0.0f) { mix.reset (v[FxTremMix]); depth.reset (v[FxTremDepth]); }
            const int mode = v.i (FxTremMode), s = v.i (FxTremShape);
            const double inc = lfo.begin (v.b (FxTremSync), v[FxTremRate], v.i (FxTremNote), c, sampleRate);
            const float stereo = v[FxTremStereo] / 360.0f;
            // Smoothing: 0 = hard edges, 1 = soft (a one-pole on the LFO, relative to the rate)
            const float rateHz = (float) (inc * sampleRate);
            const float sc = onePoleCoef (std::max (2.0f, rateHz * (60.0f - 56.0f * v[FxTremSmooth])), sampleRate);

            for (int i = 0; i < b.n; ++i)
            {
                const float before = (float) lfo.phase;
                const float ph = lfo.advance (inc);
                if (ph < before) randoms = { rnd.next01(), rnd.next01() };
                const float d = depth.process (v[FxTremDepth]);
                const float m = mix.process (v[FxTremMix]);
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    const float target = shape (s, ph + (ch == 1 ? stereo : 0.0f), randoms[ch]);
                    smooth[ch] += (target - smooth[ch]) * sc;
                }
                float l = b.l[i], r = b.r[i];
                float wl = l, wr = r;
                if (mode == 1)
                {
                    // Auto-pan: the LFO moves the image from left to right
                    const float pos = (smooth[0] * 2.0f - 1.0f) * d;
                    const auto g = dsp::panGains (pos);
                    wl = l * g[0];
                    wr = r * g[1];
                }
                else if (mode == 2)
                {
                    // Harmonic: lows and highs pulse against each other
                    const float ll = low[0].process (l, 0), lr = low[1].process (r, 0);
                    const float hl = high[0].process (l, 0), hr = high[1].process (r, 0);
                    const float gl0 = 1.0f - d * smooth[0], gh0 = 1.0f - d * (1.0f - smooth[0]);
                    const float gl1 = 1.0f - d * smooth[1], gh1 = 1.0f - d * (1.0f - smooth[1]);
                    wl = ll * gl0 + hl * gh0;
                    wr = lr * gl1 + hr * gh1;
                }
                else
                {
                    wl = l * (1.0f - d * (1.0f - smooth[0]));
                    wr = r * (1.0f - d * (1.0f - smooth[1]));
                }
                b.l[i] = l + (wl - l) * m;
                b.r[i] = r + (wr - r) * m;
            }
        }

        float getPhase() const { return (float) lfo.phase; }
        float getLevel() const { return smooth[0]; }

    private:
        float sampleRate = 44100.0f;
        LfoPhase lfo;
        dsp::Random rnd;
        std::array<float, 2> smooth {}, randoms {};
        std::array<Biquad, 2> low, high;
        Smoother mix, depth;
    };

    // ---------------------------------------------------------------------------------------------
    // Frequency shifter: an analytic signal (two allpass chains 90 degrees apart) multiplied by a complex oscillator
    // moves every partial by the same number of Hz. Ring mode multiplies by a sine instead.

    class FrequencyShifter
    {
    public:
        void prepare (double sr, int) { sampleRate = (float) sr; mix.setTime (0.02f, sampleRate); shift.setTime (0.03f, sampleRate); reset(); }

        void reset()
        {
            for (auto& h : hilbert) h = {};
            phases = { 0.0, 0.0 };
            feedback = { 0.0f, 0.0f };
            shift.reset (-1.0e9f);
            mix.reset (-1.0f);
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            if (mix.value < 0.0f) mix.reset (v[FxShiftMix]);
            if (shift.value < -1.0e8f) shift.reset (v[FxShiftAmount]);
            const bool ring = v.i (FxShiftMode) == 1;
            const float fb = v[FxShiftFeedback], spread = v[FxShiftSpread];
            float* io[] = { b.l, b.r };
            for (int i = 0; i < b.n; ++i)
            {
                const float hz = shift.process (v[FxShiftAmount]);
                const float m = mix.process (v[FxShiftMix]);
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    // Stereo: the right channel shifts the other way at full spread
                    const float f = ch == 0 ? hz : hz * (1.0f - 2.0f * spread);
                    phases[ch] += (double) f / sampleRate;
                    phases[ch] -= std::floor (phases[ch]);
                    const float ph = kTwoPi * (float) phases[ch];
                    const float x = io[ch][i];
                    const float in = x + feedback[ch] * fb;
                    float y;
                    if (ring)
                        y = in * std::sin (ph);
                    else
                    {
                        float re, im;
                        hilbert[ch].process (in, re, im);
                        y = re * std::cos (ph) - im * std::sin (ph);
                    }
                    feedback[ch] = softClip (y);
                    io[ch][i] = x * (1.0f - m) + y * m;
                }
            }
            lastPhase = (float) phases[0];
        }

        float lastPhase = 0.0f;

    private:
        // Two chains of four allpass sections (Niemitalo's 90 degree phase difference design)
        struct Hilbert
        {
            std::array<float, 4> x1 {}, x2 {}, y1 {}, y2 {};   // path A states (x[n-1], x[n-2] ...)
            std::array<float, 4> u1 {}, u2 {}, w1 {}, w2 {};   // path B states
            float delayed = 0.0f;

            static float section (float x, float a2, float& xm1, float& xm2, float& ym1, float& ym2)
            {
                const float y = a2 * (x + ym2) - xm2;
                xm2 = xm1; xm1 = x;
                ym2 = ym1; ym1 = y;
                return y;
            }

            void process (float x, float& re, float& im)
            {
                static constexpr float a[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f };
                static constexpr float c[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
                float pa = x, pb = x;
                for (size_t k = 0; k < 4; ++k)
                {
                    pa = section (pa, a[k] * a[k], x1[k], x2[k], y1[k], y2[k]);
                    pb = section (pb, c[k] * c[k], u1[k], u2[k], w1[k], w2[k]);
                }
                re = delayed;   // path A one sample later
                delayed = pa;
                im = pb;
            }
        };

        float sampleRate = 44100.0f;
        std::array<Hilbert, 2> hilbert;
        std::array<double, 2> phases {};
        std::array<float, 2> feedback {};
        Smoother mix, shift;
    };

    // ---------------------------------------------------------------------------------------------
    // Exciter: the band above a frequency is saturated (odd harmonics like tape <-> even like a tube) and the new
    // harmonics are blended back in.

    class Exciter
    {
    public:
        void prepare (double sr, int) { sampleRate = sr; amount.setTime (0.02f, (float) sr); reset(); lastFreq = -1.0f; }
        void reset() { for (auto& f : pre) f.reset(); for (auto& f : post) f.reset(); amount.reset (-1.0f); }

        // The harmonic generator (shared with the graph): colour -1 odd .. +1 even
        static float shape (float x, float colour)
        {
            const float odd = std::tanh (x);
            const float even = std::tanh (x + 0.5f * x * x) - 0.0f;
            const float t = 0.5f + 0.5f * colour;
            return odd * (1.0f - t) + even * t;
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            if (! juce::exactlyEqual (v[FxExcFreq], lastFreq))
            {
                lastFreq = v[FxExcFreq];
                pre[0].highpass (sampleRate, lastFreq, 0.7071);
                pre[1].highpass (sampleRate, lastFreq, 0.7071);
                post[0].highpass (sampleRate, lastFreq * 0.8, 0.7071);
                post[1].highpass (sampleRate, lastFreq * 0.8, 0.7071);
            }
            if (amount.value < 0.0f) amount.reset (v[FxExcAmount]);
            const float drive = 1.0f + v[FxExcDrive] * 14.0f, colour = v[FxExcColor];
            float* io[] = { b.l, b.r };
            float level = 0.0f;
            for (int i = 0; i < b.n; ++i)
            {
                const float a = amount.process (v[FxExcAmount]);
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float x = io[ch][i];
                    const float band = pre[(size_t) ch].process (x, ch);
                    const float harm = post[(size_t) ch].process (shape (band * drive, colour) / std::sqrt (drive), ch);
                    io[ch][i] = x + harm * a * 3.0f;   // amount 1 adds the new harmonics at about twice the level of the band
                    level = std::max (level, std::abs (harm));
                }
            }
            lastLevel = level;
        }

        float lastLevel = 0.0f;

    private:
        double sampleRate = 44100.0;
        float lastFreq = -1.0f;
        std::array<Biquad, 2> pre, post;
        Smoother amount;
    };

    // ---------------------------------------------------------------------------------------------
    // Harmonizer: two pitched voices (semitones), detuned against each other between left and right, with a delay
    // before them, feedback (cascading pitches) and a tilt tone on the voices.

    class Harmonizer
    {
    public:
        void prepare (double sr, int)
        {
            sampleRate = (float) sr;
            for (auto& s : shifters) s.prepare (sr, 70.0f);
            for (auto& d : lines) d.prepare ((int) (0.15 * sr));
            mix.setTime (0.02f, sampleRate);
            delay.setTime (0.05f, sampleRate);
            reset();
        }

        void reset()
        {
            for (auto& s : shifters) s.clear();
            for (auto& d : lines) d.clear();
            feedback = { 0.0f, 0.0f };
            tilt = { 0.0f, 0.0f };
            delay.reset (-1.0f);
            mix.reset (-1.0f);
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const float detune = v[FxHarmDetune] / 100.0f * 0.5f;
            const float semis[2] = { (float) v.i (FxHarmPitch1), (float) v.i (FxHarmPitch2) };
            const float levels[2] = { v[FxHarmLevel1], v[FxHarmLevel2] };
            std::array<std::array<float, 2>, 2> ratios {};   // [voice][channel]
            for (size_t k = 0; k < 2; ++k)
                for (size_t ch = 0; ch < 2; ++ch)
                    ratios[k][ch] = std::exp2 ((semis[k] + (ch == 0 ? -detune : detune) * (k == 0 ? 1.0f : -1.0f)) / 12.0f);
            const float fb = v[FxHarmFeedback], t = v[FxHarmTone];
            const float lowG = 1.0f - std::max (0.0f, t) * 0.7f, highG = 1.0f + t * (t > 0.0f ? 0.8f : 0.9f);
            const float tc = onePoleCoef (1200.0f, sampleRate);
            if (delay.value < 0.0f) delay.reset (v[FxHarmDelay]);
            if (mix.value < 0.0f) mix.reset (v[FxHarmMix]);
            float* io[] = { b.l, b.r };

            for (int i = 0; i < b.n; ++i)
            {
                const float m = mix.process (v[FxHarmMix]);
                const float d = std::max (1.0f, delay.process (v[FxHarmDelay]) * 0.001f * sampleRate);
                for (size_t ch = 0; ch < 2; ++ch)
                {
                    const float x = io[ch][i];
                    lines[ch].push (x + feedback[ch] * fb);
                    const float in = lines[ch].read (d);
                    float y = 0.0f;
                    for (size_t k = 0; k < 2; ++k)
                        if (levels[k] > 0.0001f)
                            y += shifters[k * 2 + ch].process (in, ratios[k][ch]) * levels[k];
                        else
                            shifters[k * 2 + ch].process (in, ratios[k][ch]);
                    tilt[ch] += (y - tilt[ch]) * tc;
                    y = tilt[ch] * lowG + (y - tilt[ch]) * highG;
                    feedback[ch] = softClip (y);
                    io[ch][i] = x * (1.0f - 0.5f * m) + y * m;
                }
            }
        }

    private:
        float sampleRate = 44100.0f;
        std::array<dsp::PitchShifter, 4> shifters;
        std::array<DelayLine, 2> lines;
        std::array<float, 2> feedback {}, tilt {};
        Smoother mix, delay;
    };

    // ---------------------------------------------------------------------------------------------
    // Clipper at the very end: soft, hard or tape clipping with drive and ceiling (not reorderable)

    class Clipper
    {
    public:
        void prepare (double, int) {}
        void reset() { lastOver = 0.0f; }

        static float shape (int mode, float x)
        {
            switch (mode)
            {
                case 1:  return std::clamp (x, -1.0f, 1.0f);
                case 2:  return std::tanh (x);
                default: return softClip (x * 1.15f) / softClip (1.15f);
            }
        }

        void process (StereoBuffer b, const Values& v, const Context&)
        {
            const int mode = v.i (FxOutMode);
            const float drive = juce::Decibels::decibelsToGain (v[FxOutDrive]);
            const float ceil = juce::Decibels::decibelsToGain (v[FxOutCeiling]);
            float over = 0.0f;
            for (int i = 0; i < b.n; ++i)
            {
                const float xl = b.l[i] * drive / ceil, xr = b.r[i] * drive / ceil;
                b.l[i] = std::clamp (shape (mode, xl), -1.0f, 1.0f) * ceil;
                b.r[i] = std::clamp (shape (mode, xr), -1.0f, 1.0f) * ceil;
                over = std::max (over, std::max (std::abs (xl), std::abs (xr)));
            }
            lastOver = over;
        }

        float lastOver = 0.0f;
    };
}
