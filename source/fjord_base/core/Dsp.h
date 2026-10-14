#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <complex>

// Small DSP building blocks shared by all plugins and the effects.
namespace fjord::base::dsp
{
    constexpr float kPi    = 3.14159265358979f;
    constexpr float kTwoPi = 6.28318530717959f;

    inline float noteToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }
    inline float hzToNote (float hz)   { return 69.0f + 12.0f * std::log2 (std::max (1.0f, hz) / 440.0f); }

    // Soft saturation (Padé approximation of tanh), sounds round and is cheap.
    inline float softClip (float x)
    {
        x = std::clamp (x, -3.0f, 3.0f);
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    // Small, fast random generator (xorshift32).
    struct Random
    {
        uint32_t state = 0x12345678u;

        void seed (uint32_t s) { state = s ? s : 0x9e3779b9u; }

        uint32_t nextInt()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }

        float next01()      { return (float) (nextInt() >> 8) * (1.0f / 16777216.0f); }
        float nextBipolar() { return next01() * 2.0f - 1.0f; }
    };

    // One-pole smoother for parameters (prevents zipper noise).
    struct Smoother
    {
        float value = 0.0f, coef = 0.0f;

        void setTime (float seconds, float sampleRate)
        {
            coef = std::exp (-1.0f / std::max (1.0f, seconds * sampleRate));
        }

        void reset (float v) { value = v; }
        float process (float target) { return value = target + (value - target) * coef; }
    };

    inline float onePoleCoef (float hz, float sr) { return 1.0f - std::exp (-kTwoPi * std::min (hz, sr * 0.45f) / sr); }

    inline void equalPowerMix (float mix, float& dryGain, float& wetGain)
    {
        dryGain = std::cos (mix * kPi * 0.5f);
        wetGain = std::sin (mix * kPi * 0.5f);
    }

    // Topology-preserving state variable filter (Zavalishin): stable under fast modulation.
    struct Svf
    {
        enum Type { Low, High, Band, Notch };
        float ic1 = 0.0f, ic2 = 0.0f;

        void reset() { ic1 = ic2 = 0.0f; }

        float process (float x, float cutoff, float reso, float sampleRate, int type)
        {
            const float fc = std::clamp (cutoff, 10.0f, sampleRate * 0.45f);
            const float g = std::tan (kPi * fc / sampleRate);
            const float k = 2.0f - 1.96f * std::clamp (reso, 0.0f, 1.0f);
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1, a3 = g * a2;
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            switch (type)
            {
                case High:  return x - k * v1 - v2;
                case Band:  return v1 * k;
                case Notch: return x - k * v1;
                default:    return v2;
            }
        }
    };

    // PolyBLEP residual for band-limited saw / square edges.
    inline float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    // Equal-power pan: -1 left, +1 right
    inline std::array<float, 2> panGains (float pan)
    {
        const float a = (std::clamp (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * kPi;
        return { std::cos (a) * 1.41421356f, std::sin (a) * 1.41421356f };
    }

    // Power-of-two ring buffer with cubic (Catmull-Rom) interpolation for fractional reads.
    class DelayLine
    {
    public:
        void prepare (int maxSamples)
        {
            int size = 1;
            while (size < maxSamples + 4) size <<= 1;
            buffer.assign ((size_t) size, 0.0f);
            mask = size - 1;
            write = 0;
        }

        void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }
        void push (float x) { buffer[(size_t) write] = x; write = (write + 1) & mask; }

        float read (float delay) const
        {
            const float pos = (float) write - delay;
            const int i = (int) std::floor (pos);
            const float f = pos - (float) i;
            const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
            const float c1 = 0.5f * (x1 - xm1);
            const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
            const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
            return ((c3 * f + c2) * f + c1) * f + x0;
        }

        int maxDelay() const { return mask - 4; }

    private:
        float at (int i) const { return buffer[(size_t) (i & mask)]; }
        std::vector<float> buffer;
        int mask = 0, write = 0;
    };

    // Stereo biquad (RBJ cookbook designs); the magnitude response feeds the graphs.
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<float, 2> z1 {}, z2 {};

        float process (float x, int ch)
        {
            const float y = b0 * x + z1[(size_t) ch];
            z1[(size_t) ch] = b1 * x - a1 * y + z2[(size_t) ch];
            z2[(size_t) ch] = b2 * x - a2 * y;
            return y;
        }

        void reset() { z1.fill (0.0f); z2.fill (0.0f); }

        void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
        {
            b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
            a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
        }

        static constexpr double pi = 3.14159265358979323846;

        void highpass (double sr, double f, double q)
        {
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a);
        }

        void lowpass (double sr, double f, double q)
        {
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a);
        }

        void bandpass (double sr, double f, double q)
        {
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set (a, 0, -a, 1 + a, -2 * c, 1 - a);
        }

        void notch (double sr, double f, double q)
        {
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set (1, -2 * c, 1, 1 + a, -2 * c, 1 - a);
        }

        void allpass (double sr, double f, double q)
        {
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set (1 - a, -2 * c, 1 + a, 1 + a, -2 * c, 1 - a);
        }

        void peak (double sr, double f, double q, double gainDb)
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w = 2.0 * pi * f / sr, c = std::cos (w), a = std::sin (w) / (2.0 * q);
            set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
        }

        void shelf (double sr, double f, double gainDb, bool high)
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w = 2.0 * pi * f / sr, c = std::cos (w), s = std::sin (w);
            const double a = s / 2.0 * std::sqrt (2.0), sq = 2.0 * std::sqrt (A) * a;
            if (high)
                set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                     (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
            else
                set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                     (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
        }

        double magnitude (double freq, double sr) const
        {
            const std::complex<double> z = std::polar (1.0, -2.0 * pi * freq / sr);
            const auto num = (double) b0 + (double) b1 * z + (double) b2 * z * z;
            const auto den = 1.0 + (double) a1 * z + (double) a2 * z * z;
            return std::abs (num / den);
        }
    };

    // Pitch shifter: two crossfading read heads sweeping through a short delay (shimmer, grains, harmonizer).
    class PitchShifter
    {
    public:
        void prepare (double sr, float windowMs = 60.0f)
        {
            line.prepare ((int) (sr * 0.25));
            window = std::max (64.0f, windowMs * 0.001f * (float) sr);
            phase = 0.0f;
        }

        void clear() { line.clear(); phase = 0.0f; }

        float process (float x, float ratio)
        {
            line.push (x);
            phase += (1.0f - ratio) / window;
            phase -= std::floor (phase);
            const float p2 = phase + 0.5f - std::floor (phase + 0.5f);
            const float a = line.read (2.0f + phase * window), b = line.read (2.0f + p2 * window);
            const float wa = std::sin (kPi * phase), wb = std::sin (kPi * p2);
            return a * wa * wa + b * wb * wb;
        }

    private:
        DelayLine line;
        float window = 2048.0f, phase = 0.0f;
    };

    // Stereo audio as two raw channel pointers (no allocation, works on parts of a block).
    struct StereoBuffer
    {
        float* l;
        float* r;
        int n;
    };
}
