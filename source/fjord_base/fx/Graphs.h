#pragma once
#include "Chain.h"
#include "../ui/Controls.h"

// Pictures of every effect: small on the chain tiles, big (with grid and labels) in the editor.
namespace fjord::base::fx
{
    using ui::Theme;
    using ui::strokeGlow;
    using ui::freqToX;
    using ui::xToFreq;

    // Live state the graphs animate with (one per tile and one for the big graph).
    struct Anim
    {
        struct Particle { float x, y, vx, vy, life; };
        std::vector<Particle> particles;
        std::vector<float> history;      // compressor reduction / transient gain / squash activity over time
        std::vector<float> echoes;       // ages of delay echo rings
        uint32_t lastTriggers = 0;
        float time = 0.0f, level = 0.0f, inLevel = 0.0f;
        juce::Random rnd { 3 };

        void tick (const Meters& m, const Values& v, float dt, int module)
        {
            time += dt;
            const auto t = m.triggers.load();
            const float inNow = m.in[(size_t) module].load(), outNow = m.out[(size_t) module].load();
            // A trigger: a note / hit counted by the chain, or a jump in the input level (single-effect plugins)
            const bool hit = t != lastTriggers || (inNow > 0.05f && inNow > inLevel * 2.5f);
            lastTriggers = t;
            inLevel = std::max (inNow, inLevel * 0.85f);
            level = std::max (outNow, level * 0.9f);

            if (module == (int) Module::Reverb)
            {
                const bool shimmer = v.i (FxVerbMode) == 3;
                if (shimmer && outNow > 0.01f && rnd.nextFloat() < 0.6f)
                    particles.push_back ({ 0.2f + 0.6f * rnd.nextFloat(), 0.9f, (rnd.nextFloat() - 0.5f) * 0.05f, -0.15f - 0.25f * rnd.nextFloat(), 1.0f });
                if (hit)
                    for (int i = 0; i < 40; ++i)
                    {
                        const float a = rnd.nextFloat() * kTwoPi, s = 0.15f + rnd.nextFloat() * 0.6f;
                        particles.push_back ({ 0.5f, 0.55f, std::cos (a) * s * 0.8f, std::sin (a) * s * 0.56f, 1.0f });
                    }
                const float decay = v.b (FxVerbFreeze) ? 1000.0f : v[FxVerbDecay];
                for (auto& p : particles)
                {
                    p.x += p.vx * dt; p.y += p.vy * dt;
                    p.vx *= 0.97f; p.vy *= 0.97f;
                    if (p.x < 0.04f || p.x > 0.96f) p.vx = -p.vx;
                    if (p.y < 0.08f || p.y > 0.92f) p.vy = -p.vy;
                    p.life -= dt / std::max (0.2f, decay);
                }
                particles.erase (std::remove_if (particles.begin(), particles.end(), [] (const Particle& p) { return p.life <= 0.0f; }), particles.end());
                if (particles.size() > 400) particles.erase (particles.begin(), particles.begin() + (long) (particles.size() - 400));
            }
            if (module == (int) Module::Delay)
            {
                if (hit) echoes.push_back (0.0f);
                for (auto& e : echoes) e += dt;
                echoes.erase (std::remove_if (echoes.begin(), echoes.end(), [] (float e) { return e > 6.0f; }), echoes.end());
            }
            if (module == (int) Module::Compressor || module == (int) Module::Transient || module == (int) Module::Squash)
            {
                const float value = module == (int) Module::Compressor ? m.compReduction.load()
                                  : module == (int) Module::Transient ? m.transientGain.load() : m.squash.load();
                history.push_back (value);
                if (history.size() > 160) history.erase (history.begin());
            }
        }
    };

    // Draws one effect's picture. 'mini' = the small version on the tiles.
    inline void drawGraph (juce::Graphics& g, int module, juce::Rectangle<float> r, const Values& v, const Meters& meters,
                           Anim& anim, bool mini)
    {
        const auto& p = Theme::get().palette();
        const bool dark = Theme::get().isDark();
        const bool on = v.b (onParam (module));
        const auto col = on ? p.accent : p.textDim;
        const float lw = mini ? 1.3f : 1.8f;
        const double sampleRate = meters.sampleRate.load();

        auto gridLines = [&] (int nx, int ny)
        {
            if (mini) return;
            g.setColour (p.line.withAlpha (0.5f));
            for (int i = 1; i < nx; ++i) g.fillRect (r.getX() + r.getWidth() * (float) i / (float) nx, r.getY(), 1.0f, r.getHeight());
            for (int i = 1; i < ny; ++i) g.fillRect (r.getX(), r.getY() + r.getHeight() * (float) i / (float) ny, r.getWidth(), 1.0f);
        };

        auto label = [&] (const juce::String& text)
        {
            if (mini) return;
            g.setColour (p.textDim);
            g.setFont (ui::font (9.5f, ui::Weight::Medium));
            g.drawText (text, r.reduced (6.0f), juce::Justification::topLeft);
        };

        auto history = [&] (float range, bool fromTop)
        {
            juce::Path trace;
            const auto& h = anim.history;
            for (size_t i = 0; i < h.size(); ++i)
            {
                const float x = r.getX() + r.getWidth() * (float) i / 159.0f + r.getWidth() * (float) (160 - h.size()) / 159.0f;
                const float y = fromTop ? r.getY() + juce::jlimit (0.0f, 1.0f, h[i] / range) * r.getHeight()
                                        : r.getCentreY() - juce::jlimit (-1.0f, 1.0f, h[i] / range) * r.getHeight() * 0.5f;
                if (i == 0) trace.startNewSubPath (x, y); else trace.lineTo (x, y);
            }
            if (! fromTop)
            {
                g.setColour (p.line);
                g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.0f);
            }
            if (! h.empty())
            {
                if (fromTop)
                {
                    juce::Path fill (trace);
                    fill.lineTo (r.getRight(), r.getY());
                    fill.lineTo (r.getX() + r.getWidth() * (float) (160 - h.size()) / 159.0f, r.getY());
                    fill.closeSubPath();
                    g.setColour (col.withAlpha (0.15f));
                    g.fillPath (fill);
                }
                strokeGlow (g, trace, col, lw, ! mini);
            }
        };

        // A response curve on the log frequency axis (dB from the given function)
        auto response = [&] (auto&& dbAt, float range, juce::Colour c, float width, bool fill)
        {
            juce::Path curve;
            const int n = std::max (40, (int) r.getWidth());
            for (int i = 0; i <= n; ++i)
            {
                const float x = r.getX() + r.getWidth() * (float) i / (float) n;
                const float db = (float) dbAt (xToFreq (x, r));
                const float y = r.getCentreY() - juce::jlimit (-range, range, db) / range * r.getHeight() * 0.5f;
                if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
            }
            if (fill)
            {
                juce::Path f (curve);
                f.lineTo (r.getRight(), r.getCentreY());
                f.lineTo (r.getX(), r.getCentreY());
                f.closeSubPath();
                g.setColour (c.withAlpha (0.12f));
                g.fillPath (f);
            }
            strokeGlow (g, curve, c, width, ! mini);
        };

        auto freqGrid = [&]
        {
            if (mini) return;
            g.setColour (p.line.withAlpha (0.5f));
            for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
                g.fillRect (freqToX (hz, r), r.getY(), 1.0f, r.getHeight());
        };

        switch ((Module) module)
        {
            case Module::Saturation:
            {
                gridLines (4, 4);
                const int mode = v.i (FxSatMode);
                const float gain = Saturation::driveGain (v[FxSatDrive]);
                const float comp = Saturation::compensation (v[FxSatDrive]);
                const float out = juce::Decibels::decibelsToGain (v[FxSatOut]);
                juce::Path curve;
                for (int i = 0; i <= 100; ++i)
                {
                    const float x = -1.0f + 2.0f * (float) i / 100.0f;
                    const float y = juce::jlimit (-1.2f, 1.2f, Saturation::shape (mode, x * gain, v[FxSatBias]) * comp * out);
                    const auto pt = juce::Point<float> (r.getX() + (x + 1.0f) * 0.5f * r.getWidth(), r.getCentreY() - y * r.getHeight() * 0.42f);
                    if (i == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
                }
                g.setColour (p.line);
                g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 0.8f);
                strokeGlow (g, curve, col, lw, ! mini);
                // The signal riding the curve
                const float x = juce::jlimit (0.0f, 1.0f, anim.inLevel);
                const float y = Saturation::shape (mode, x * gain, v[FxSatBias]) * comp;
                const auto dot = juce::Point<float> (r.getX() + (x + 1.0f) * 0.5f * r.getWidth(), r.getCentreY() - juce::jlimit (-1.2f, 1.2f, y) * r.getHeight() * 0.42f);
                if (on && x > 0.01f)
                {
                    g.setColour (col.withAlpha (0.3f));
                    g.fillEllipse (juce::Rectangle<float> (mini ? 10.0f : 18.0f, mini ? 10.0f : 18.0f).withCentre (dot));
                    g.setColour (col);
                    g.fillEllipse (juce::Rectangle<float> (mini ? 4.0f : 7.0f, mini ? 4.0f : 7.0f).withCentre (dot));
                }
                break;
            }
            case Module::Lofi:
            {
                // A tape-wobbled wave, stepped by the bit depth and the sample-rate reduction
                gridLines (4, 4);
                const float q = std::pow (2.0f, std::min (v[FxLofiBits], 8.0f) - 1.0f);
                const float rate = v[FxLofiRate];
                const float wob = meters.lofiWow.load();
                juce::Path wave, steps;
                float hold = 0.0f, ph = 1.0f;
                for (int i = 0; i <= 200; ++i)
                {
                    const float t = (float) i / 200.0f;
                    const float bend = v[FxLofiWow] * 0.08f * std::sin (kTwoPi * (t * 0.7f + wob));
                    const float s = std::sin ((t + bend) * kTwoPi * 2.5f) * (0.75f + 0.25f * std::sin (kTwoPi * (t * 9.0f) * v[FxLofiFlutter]));
                    ph += 1.0f / std::max (1.0f, rate * 0.4f);
                    if (ph >= 1.0f) { ph -= 1.0f; hold = std::round (s * q) / q; }
                    const float x = r.getX() + t * r.getWidth();
                    if (i == 0) { wave.startNewSubPath (x, r.getCentreY() - s * r.getHeight() * 0.4f); steps.startNewSubPath (x, r.getCentreY() - hold * r.getHeight() * 0.4f); }
                    wave.lineTo (x, r.getCentreY() - s * r.getHeight() * 0.4f);
                    steps.lineTo (x, r.getCentreY() - hold * r.getHeight() * 0.4f);
                }
                g.setColour (p.textDim.withAlpha (0.35f));
                g.strokePath (wave, juce::PathStrokeType (1.0f));
                strokeGlow (g, steps, col, lw, ! mini);
                juce::Random dust ((juce::int64) (wob * 1000.0f));
                const int specks = (int) (v[FxLofiNoise] * (mini ? 20.0f : 80.0f));
                for (int i = 0; i < specks; ++i)
                {
                    g.setColour (p.textDim.withAlpha (0.25f + 0.3f * dust.nextFloat()));
                    g.fillEllipse (r.getX() + dust.nextFloat() * r.getWidth(), r.getY() + dust.nextFloat() * r.getHeight(), 1.4f, 1.4f);
                }
                break;
            }
            case Module::Transient:
                gridLines (6, 4);
                history (12.0f, false);
                label ("GAIN");
                break;
            case Module::Compressor:
            {
                if (! mini)
                {
                    // Static curve on the left, reduction history on the right
                    auto curveArea = r.removeFromLeft (r.getHeight()).reduced (4.0f);
                    r.removeFromLeft (16.0f);
                    g.setColour (p.line.withAlpha (0.5f));
                    g.drawRect (curveArea, 1.0f);
                    juce::Path curve;
                    for (int i = 0; i <= 60; ++i)
                    {
                        const float inDb = -60.0f + (float) i;
                        const float outDb = inDb - Compressor::reduction (inDb, v[FxCompThresh], v[FxCompRatio], v[FxCompKnee]);
                        const auto pt = juce::Point<float> (curveArea.getX() + (inDb + 60.0f) / 60.0f * curveArea.getWidth(),
                                                            curveArea.getBottom() - (outDb + 60.0f) / 60.0f * curveArea.getHeight());
                        if (i == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
                    }
                    g.setColour (p.line);
                    g.drawLine (curveArea.getX(), curveArea.getBottom(), curveArea.getRight(), curveArea.getY(), 0.8f);
                    strokeGlow (g, curve, col, lw, true);
                    const float tx = curveArea.getX() + (v[FxCompThresh] + 60.0f) / 60.0f * curveArea.getWidth();
                    g.setColour (p.textDim.withAlpha (0.6f));
                    g.fillRect (tx, curveArea.getY(), 1.0f, curveArea.getHeight());
                }
                gridLines (6, 4);
                history (18.0f, true);
                label ("GAIN REDUCTION");
                break;
            }
            case Module::Squash:
            {
                // Three bands: quiet parts pushed up, loud parts pushed down
                gridLines (3, 4);
                const float act = anim.history.empty() ? 0.0f : anim.history.back();
                for (int k = 0; k < 3; ++k)
                {
                    auto band = r.withWidth (r.getWidth() / 3.0f).translated (r.getWidth() / 3.0f * (float) k, 0.0f).reduced (mini ? 3.0f : 10.0f, 4.0f);
                    const float wobble = 0.5f + 0.5f * std::sin (anim.time * (2.0f + (float) k) + (float) k);
                    const float h = band.getHeight() * (0.3f + 0.4f * v[FxSqDepth] * wobble * std::min (1.0f, act / 6.0f + 0.2f));
                    const auto c = ui::gradientAt (dark, (float) k / 2.0f).withAlpha (on ? 0.85f : 0.35f);
                    g.setColour (c.withAlpha (0.18f));
                    g.fillRoundedRectangle (band, 3.0f);
                    g.setColour (c);
                    g.fillRoundedRectangle (band.withSizeKeepingCentre (band.getWidth(), h), 3.0f);
                    if (! mini)
                    {
                        juce::Path arrows;
                        const float cx = band.getCentreX();
                        arrows.addTriangle (cx - 5.0f, band.getY() + 8.0f, cx + 5.0f, band.getY() + 8.0f, cx, band.getY() + 16.0f);
                        if (v[FxSqUpward] > 0.05f)
                            arrows.addTriangle (cx - 5.0f, band.getBottom() - 8.0f, cx + 5.0f, band.getBottom() - 8.0f, cx, band.getBottom() - 16.0f);
                        g.setColour (p.text.withAlpha (0.5f));
                        g.fillPath (arrows);
                    }
                }
                break;
            }
            case Module::Eq:
            {
                EqDesign eq;
                eq.design (sampleRate, v);
                freqGrid();
                if (! mini)
                {
                    g.setColour (p.line.withAlpha (0.5f));
                    for (float db : { -12.0f, 0.0f, 12.0f })
                        g.fillRect (r.getX(), r.getCentreY() - db / 20.0f * r.getHeight() * 0.5f, r.getWidth(), 1.0f);
                }
                response ([&] (float f) { return eq.magnitudeDb (f, sampleRate); }, 20.0f, col, lw, true);
                break;
            }
            case Module::Filter:
            {
                freqGrid();
                gridLines (1, 4);
                const int t = v.i (FxFiltType);
                auto curveFor = [&] (float cutoff, juce::Colour c, float width)
                {
                    Biquad b1, b2;
                    const double q = 0.6 + v[FxFiltReso] * 6.0;
                    const double fc = juce::jlimit (20.0, sampleRate * 0.45, (double) cutoff);
                    switch (t)
                    {
                        case 0: b1.lowpass (sampleRate, fc, 0.54); b2.lowpass (sampleRate, fc, 0.6 + v[FxFiltReso] * 5.0); break;
                        case 1: b1.lowpass (sampleRate, fc, q); break;
                        case 2: b1.bandpass (sampleRate, fc, q); break;
                        case 3: b1.highpass (sampleRate, fc, q); break;
                        default: b1.notch (sampleRate, fc, q * 0.5); break;
                    }
                    juce::Path path;
                    const int n = std::max (40, (int) r.getWidth());
                    for (int i = 0; i <= n; ++i)
                    {
                        const float x = r.getX() + r.getWidth() * (float) i / (float) n;
                        const double f = xToFreq (x, r);
                        double mag = b1.magnitude (f, sampleRate);
                        if (t == 0) mag *= b2.magnitude (f, sampleRate);
                        const float db = (float) (20.0 * std::log10 (std::max (1.0e-6, mag)));
                        const float y = r.getY() + r.getHeight() * 0.3f - juce::jlimit (-60.0f, 24.0f, db) / 60.0f * r.getHeight() * 0.7f;
                        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, std::min (y, r.getBottom()));
                    }
                    strokeGlow (g, path, c, width, ! mini);
                };
                const float base = v[FxFiltCutoff];
                const float live = meters.filterCutoff.load();
                if (std::abs (v[FxFiltEnv]) > 0.01f || v[FxFiltLfoDepth] > 0.01f)
                {
                    curveFor (base * std::exp2 (v[FxFiltEnv] + v[FxFiltLfoDepth]), col.withAlpha (0.25f), 1.0f);
                    if (on) curveFor (live, col.withAlpha (0.6f), 1.2f);
                }
                curveFor (base, col, lw);
                break;
            }
            case Module::Motion:
            {
                gridLines (4, 2);
                const int mode = v.i (FxMotMode);
                const float depth = v[FxMotDepth];
                const float phase = meters.motionPhase.load();
                juce::Path wave;
                for (int i = 0; i <= 120; ++i)
                {
                    const float x = (float) i / 120.0f;
                    const float lfo = 0.5f - 0.5f * std::cos (kTwoPi * (x * 2.0f + phase));
                    const float y = mode == 2 ? std::sin (x * 40.0f * (0.6f + lfo * depth)) * 0.6f
                                  : std::sin (x * kTwoPi * 6.0f + lfo * depth * 4.0f) * (0.4f + 0.3f * lfo);
                    const auto pt = juce::Point<float> (r.getX() + x * r.getWidth(), r.getCentreY() - y * r.getHeight() * 0.4f);
                    if (i == 0) wave.startNewSubPath (pt); else wave.lineTo (pt);
                }
                strokeGlow (g, wave, col, lw, ! mini);
                if (mode == 0 || mode == 1 || mode == 4)
                    for (int k = 1; k <= (mode == 1 ? 2 : 1); ++k)
                    {
                        g.setColour (col.withAlpha (0.35f / (float) k));
                        g.strokePath (wave, juce::PathStrokeType (lw * 0.8f), juce::AffineTransform::translation ((float) k * depth * 6.0f, (float) k * 3.0f));
                    }
                break;
            }
            case Module::Delay:
            {
                // Taps as bars falling with the feedback, and rings rippling out after each hit
                const float seconds = Delay::delaySeconds (v, meters.bpm.load());
                const float window = std::max (seconds * 6.0f, 0.5f);
                float amp = v[FxDlyMix] * 1.4f;
                g.setColour (p.line);
                g.fillRect (r.getX(), r.getBottom() - 1.0f, r.getWidth(), 1.0f);
                for (int k = 1; k <= 8 && seconds * (float) k < window; ++k)
                {
                    const float x = r.getX() + r.getWidth() * (seconds * (float) k / window);
                    const float h = juce::jlimit (0.03f, 1.0f, amp) * r.getHeight() * 0.85f;
                    g.setColour (col.withAlpha (0.25f + 0.6f * juce::jlimit (0.0f, 1.0f, amp)));
                    g.fillRoundedRectangle (x - 1.5f, r.getBottom() - h, 3.0f, h, 1.5f);
                    amp *= v[FxDlyFeedback];
                }
                for (float age : anim.echoes)
                    for (int k = 1; k <= 8; ++k)
                    {
                        const float rel = age - seconds * (float) k;
                        if (rel < 0.0f || rel > 0.6f) continue;
                        const float x = r.getX() + r.getWidth() * (seconds * (float) k / window);
                        const float rad = rel * (mini ? 30.0f : 80.0f);
                        const float a = (1.0f - rel / 0.6f) * std::pow (v[FxDlyFeedback], (float) (k - 1)) * (on ? 0.9f : 0.3f);
                        g.setColour (col.withAlpha (a));
                        g.drawEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre ({ x, r.getBottom() - 4.0f }), 1.0f);
                    }
                break;
            }
            case Module::Reverb:
            {
                // A room seen from above: size draws the walls, particles bounce and fade with the decay
                const float size = 0.35f + 0.65f * v[FxVerbSize];
                auto room = r.withSizeKeepingCentre (r.getWidth() * size, r.getHeight() * (0.4f + 0.6f * size));
                g.setColour (v.b (FxVerbFreeze) ? col : p.line);
                g.drawRoundedRectangle (room, mini ? 3.0f : 6.0f, 1.0f);
                for (const auto& pt : anim.particles)
                {
                    g.setColour (ui::gradientAt (dark, 1.0f - pt.life).withAlpha (std::min (1.0f, pt.life) * (on ? 0.9f : 0.3f)));
                    const float d = mini ? 2.0f : 3.0f + 2.0f * std::min (1.0f, pt.life);
                    g.fillEllipse (room.getX() + pt.x * room.getWidth() - d * 0.5f, room.getY() + pt.y * room.getHeight() - d * 0.5f, d, d);
                }
                if (anim.particles.empty())
                {
                    g.setColour (col.withAlpha (0.5f));
                    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (room.getCentre()));
                }
                break;
            }
            case Module::Tremolo:
            {
                // The LFO shape over two cycles, a dot rides it at the current phase
                gridLines (4, 2);
                const int shape = v.i (FxTremShape);
                const float depth = v[FxTremDepth];
                const float ph = meters.tremoloPhase.load();
                juce::Path curve;
                juce::Random rnd (7);
                for (int i = 0; i <= 160; ++i)
                {
                    const float x = (float) i / 160.0f;
                    const float s = Tremolo::shape (shape, x * 2.0f, shape == 4 ? (float) ((int) (x * 8.0f) * 37 % 11) / 10.0f : 0.0f);
                    const float y = 1.0f - depth * (1.0f - s);
                    const auto pt = juce::Point<float> (r.getX() + x * r.getWidth(), r.getBottom() - 4.0f - y * (r.getHeight() - 8.0f));
                    if (i == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
                }
                strokeGlow (g, curve, col, lw, ! mini);
                if (v.i (FxTremMode) == 1 && ! mini)
                {
                    // auto-pan: the position as a dot between L and R
                    const float pos = (meters.tremoloLevel.load() * 2.0f - 1.0f) * depth;
                    g.setColour (p.textDim);
                    g.setFont (ui::font (9.0f, ui::Weight::Medium));
                    g.drawText ("L", r.withWidth (14.0f), juce::Justification::centred);
                    g.drawText ("R", r.withTrimmedLeft (r.getWidth() - 14.0f), juce::Justification::centred);
                    g.setColour (col);
                    g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ r.getCentreX() + pos * r.getWidth() * 0.45f, r.getY() + 10.0f }));
                }
                if (on)
                {
                    const float x = ph * 0.5f;
                    const float s = Tremolo::shape (shape, ph, 0.5f);
                    const auto dot = juce::Point<float> (r.getX() + x * r.getWidth(), r.getBottom() - 4.0f - (1.0f - depth * (1.0f - s)) * (r.getHeight() - 8.0f));
                    g.setColour (col.withAlpha (0.3f));
                    g.fillEllipse (juce::Rectangle<float> (mini ? 9.0f : 16.0f, mini ? 9.0f : 16.0f).withCentre (dot));
                    g.setColour (col);
                    g.fillEllipse (juce::Rectangle<float> (mini ? 4.0f : 6.0f, mini ? 4.0f : 6.0f).withCentre (dot));
                }
                break;
            }
            case Module::Shifter:
            {
                // Partials of a tone (faint) and where they end up after the shift
                freqGrid();
                const float shift = v[FxShiftAmount];
                const bool ring = v.i (FxShiftMode) == 1;
                const float wobble = 0.5f + 0.5f * std::sin (kTwoPi * meters.shifterPhase.load());
                for (int k = 1; k <= 8; ++k)
                {
                    const float f = 220.0f * (float) k;
                    const float h = r.getHeight() * (0.85f / std::sqrt ((float) k));
                    g.setColour (p.textDim.withAlpha (0.3f));
                    g.fillRect (freqToX (f, r) - 0.5f, r.getBottom() - h, 1.0f, h);
                    auto bar = [&] (float hz, float alpha)
                    {
                        if (hz < 20.0f || hz > 20000.0f) return;
                        g.setColour (col.withAlpha (alpha * (on ? 1.0f : 0.5f)));
                        g.fillRoundedRectangle (freqToX (hz, r) - 1.5f, r.getBottom() - h * (0.85f + 0.15f * wobble), 3.0f, h * (0.85f + 0.15f * wobble), 1.5f);
                    };
                    if (ring) { bar (f + std::abs (shift), 0.8f); bar (f - std::abs (shift), 0.8f); }
                    else      bar (f + shift, 0.9f);
                }
                label (ring ? "RING" : (shift >= 0.0f ? "UP " : "DOWN ") + juce::String (std::abs (shift), 1) + " HZ");
                break;
            }
            case Module::Exciter:
            {
                // The band above the frequency, with new harmonics growing out of it
                freqGrid();
                const float f0 = v[FxExcFreq];
                const float x0 = freqToX (f0, r);
                g.setColour (col.withAlpha (0.08f));
                g.fillRect (juce::Rectangle<float> (x0, r.getY(), r.getRight() - x0, r.getHeight()));
                g.setColour (col.withAlpha (0.5f));
                g.fillRect (x0, r.getY(), 1.0f, r.getHeight());
                const float lvl = juce::jlimit (0.0f, 1.0f, meters.exciterLevel.load() * 6.0f);
                const float colour = v[FxExcColor];
                for (int k = 2; k <= 9; ++k)
                {
                    const float f = std::min (19000.0f, f0 * 0.7f * (float) k);
                    const bool even = k % 2 == 0;
                    const float weight = even ? 0.5f + 0.5f * colour : 0.5f - 0.5f * colour;
                    const float h = r.getHeight() * 0.8f * v[FxExcAmount] * (0.3f + 0.7f * v[FxExcDrive]) * weight / std::sqrt ((float) k - 1.0f)
                                    * (0.6f + 0.4f * (on ? lvl : 0.0f));
                    g.setColour (ui::gradientAt (dark, (float) (k - 2) / 7.0f).withAlpha (on ? 0.85f : 0.35f));
                    g.fillRoundedRectangle (freqToX (f, r) - 2.0f, r.getBottom() - h, 4.0f, h, 2.0f);
                }
                break;
            }
            case Module::Harmonizer:
            {
                // A pitch staff: the played note in the middle, the voices above / below by their intervals
                gridLines (1, 4);
                const float mid = r.getCentreY();
                const float perSemi = r.getHeight() / 52.0f;
                const float t = anim.time;
                auto line = [&] (float semis, float level, juce::Colour c)
                {
                    juce::Path path;
                    const float y = mid - semis * perSemi;
                    for (int i = 0; i <= 60; ++i)
                    {
                        const float x = (float) i / 60.0f;
                        const float wob = std::sin (kTwoPi * (x * 3.0f + t * 0.4f) + semis) * v[FxHarmDetune] * 0.04f;
                        if (i == 0) path.startNewSubPath (r.getX(), y + wob); else path.lineTo (r.getX() + x * r.getWidth(), y + wob);
                    }
                    strokeGlow (g, path, c.withMultipliedAlpha (0.3f + 0.7f * level), lw * (0.6f + level), ! mini);
                };
                line (0.0f, 0.5f, p.textDim);
                if (v[FxHarmLevel1] > 0.001f) line ((float) v.i (FxHarmPitch1), v[FxHarmLevel1], col);
                if (v[FxHarmLevel2] > 0.001f) line ((float) v.i (FxHarmPitch2), v[FxHarmLevel2], on ? ui::gradientAt (dark, 1.0f) : col);
                if (! mini)
                {
                    g.setColour (p.textDim);
                    g.setFont (ui::font (9.5f, ui::Weight::Medium));
                    auto tag = [&] (int semis) { return (semis > 0 ? "+" : "") + juce::String (semis); };
                    g.drawText (tag (v.i (FxHarmPitch1)), juce::Rectangle<float> (r.getRight() - 40.0f, mid - (float) v.i (FxHarmPitch1) * perSemi - 14.0f, 36.0f, 12.0f), juce::Justification::centredRight);
                    if (v[FxHarmLevel2] > 0.001f)
                        g.drawText (tag (v.i (FxHarmPitch2)), juce::Rectangle<float> (r.getRight() - 40.0f, mid - (float) v.i (FxHarmPitch2) * perSemi - 14.0f, 36.0f, 12.0f), juce::Justification::centredRight);
                }
                break;
            }
            case Module::Out:
            {
                gridLines (4, 4);
                const int mode = v.i (FxOutMode);
                const float drive = juce::Decibels::decibelsToGain (v[FxOutDrive]);
                const float ceil = juce::Decibels::decibelsToGain (v[FxOutCeiling]);
                juce::Path curve;
                for (int i = 0; i <= 100; ++i)
                {
                    const float x = -1.5f + 3.0f * (float) i / 100.0f;
                    const float y = juce::jlimit (-1.0f, 1.0f, Clipper::shape (mode, x * drive / ceil)) * ceil;
                    const auto pt = juce::Point<float> (r.getX() + (x + 1.5f) / 3.0f * r.getWidth(), r.getCentreY() - y * r.getHeight() * 0.45f);
                    if (i == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
                }
                g.setColour (p.line);
                for (float c : { ceil, -ceil })
                    g.fillRect (r.getX(), r.getCentreY() - c * r.getHeight() * 0.45f, r.getWidth(), 1.0f);
                strokeGlow (g, curve, col, lw, ! mini);
                const float over = meters.clipOver.load();
                if (on && over > 1.0f && ! mini)
                {
                    g.setColour (p.accent);
                    g.setFont (ui::font (10.0f, ui::Weight::SemiBold));
                    g.drawText ("CLIPPING " + juce::String (juce::Decibels::gainToDecibels (over), 1) + " dB", r.reduced (6.0f), juce::Justification::topRight);
                }
                break;
            }
            case Module::Width:
            case Module::Count:
            default:
            {
                // Stereo field: half circle, the width opens the fan; below the mono frequency everything sits in the centre
                const auto c = juce::Point<float> (r.getCentreX(), r.getBottom() - 2.0f);
                const float rad = std::min (r.getWidth() * 0.5f, r.getHeight()) - 4.0f;
                juce::Path arc;
                arc.addCentredArc (c.x, c.y, rad, rad, 0.0f, -kPi * 0.5f, kPi * 0.5f, true);
                g.setColour (p.line);
                g.strokePath (arc, juce::PathStrokeType (1.0f));
                const float spread = juce::jlimit (0.0f, 1.0f, v[FxWidthAmount] * 0.5f + v[FxWidthHaas] / 60.0f) * kPi * 0.5f;
                juce::Path fan;
                fan.startNewSubPath (c);
                fan.addCentredArc (c.x, c.y, rad, rad, 0.0f, -spread, spread, false);
                fan.closeSubPath();
                g.setColour (col.withAlpha (0.18f));
                g.fillPath (fan);
                g.setColour (col);
                g.strokePath (fan, juce::PathStrokeType (lw));
                const float mono = rad * (0.15f + 0.6f * std::log (v[FxWidthMono] / 20.0f) / std::log (25.0f));
                g.setColour (col.withAlpha (0.5f));
                g.fillEllipse (juce::Rectangle<float> (mono * 2.0f, mono * 2.0f).withCentre (c));
                break;
            }
        }
    }
}
