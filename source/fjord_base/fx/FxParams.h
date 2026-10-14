#pragma once
#include "../params/Params.h"

// The effect parameters of the whole family. Every plugin includes this block once in its own parameter table
// (FJORD_FX_PARAMS (X) inside MY_PARAMS), so effects can be modulated like any other parameter and every plugin,
// every single-effect plugin and every effect preset uses the same IDs: fx_<effect>_<param>.
//
// Rules: IDs never change, new parameters go to the END of an effect's block only if no plugin stores the block
// position (plugins store parameters by ID, so appending anywhere inside is fine), choice lists are append-only,
// ranges may only grow. A new effect is appended at the end of the table and of fx::Module.

// Note values of synced delays, LFOs and tremolos (beats = quarter notes, see fx::noteBeats)
#define FJORD_FX_NOTES "1/32|1/16T|1/16|1/8T|1/16D|1/8|1/4T|1/8D|1/4|1/2T|1/4D|1/2|1/2D|1 Bar|2 Bars"

#define FJORD_FX_PARAMS(X)                                                                                                         \
    /* Saturation */                                                                                                               \
    X (FxSatOn,       "fx_saturation_on",       "Saturation On",         B, 0, 1, 0,        0, None, "")                           \
    X (FxSatMode,     "fx_saturation_mode",     "Saturation Curve",      C, 0, 5, 0,        0, None, "Tape|Tube|Diode|Fold|Hard|Rectify") \
    X (FxSatDrive,    "fx_saturation_drive",    "Saturation Drive",      F, 0, 1, 0.3f,     0, Pct, "")                            \
    X (FxSatBias,     "fx_saturation_bias",     "Saturation Bias",       F, -1, 1, 0,       0, Bip, "")                            \
    X (FxSatTone,     "fx_saturation_tone",     "Saturation Tone",       F, -1, 1, 0,       0, Bip, "")                            \
    X (FxSatOs,       "fx_saturation_os",       "Oversampling",          C, 0, 2, 1,        0, None, "1x|2x|4x")                    \
    X (FxSatOut,      "fx_saturation_out",      "Saturation Output",     F, -24, 12, 0,     0, Db, "")                             \
    X (FxSatMix,      "fx_saturation_mix",      "Saturation Mix",        F, 0, 1, 1,        0, Pct, "")                            \
    X (FxSatAuto,     "fx_saturation_autogain", "Saturation Auto Gain",  B, 0, 1, 0,        0, None, "")                           \
    /* Lo-Fi: tape wow / flutter, age, hiss, bit depth, sample-rate reduction */                                                   \
    X (FxLofiOn,      "fx_lofi_on",             "Lo-Fi On",              B, 0, 1, 0,        0, None, "")                           \
    X (FxLofiWow,     "fx_lofi_wow",            "Lo-Fi Wow",             F, 0, 1, 0.3f,     0, Pct, "")                            \
    X (FxLofiFlutter, "fx_lofi_flutter",        "Lo-Fi Flutter",         F, 0, 1, 0.2f,     0, Pct, "")                            \
    X (FxLofiAge,     "fx_lofi_age",            "Lo-Fi Age",             F, 0, 1, 0.3f,     0, Pct, "")                            \
    X (FxLofiNoise,   "fx_lofi_noise",          "Lo-Fi Hiss",            F, 0, 1, 0.1f,     0, Pct, "")                            \
    X (FxLofiBits,    "fx_lofi_bits",           "Lo-Fi Bits",            F, 1, 16, 16,      0, Bits, "")                           \
    X (FxLofiRate,    "fx_lofi_rate",           "Lo-Fi Downsample",      F, 1, 40, 1,       6, Mult, "")                           \
    X (FxLofiJitter,  "fx_lofi_jitter",         "Lo-Fi Jitter",          F, 0, 1, 0,        0, Pct, "")                            \
    X (FxLofiMix,     "fx_lofi_mix",            "Lo-Fi Mix",             F, 0, 1, 1,        0, Pct, "")                            \
    /* Transient shaper */                                                                                                         \
    X (FxTransOn,     "fx_transient_on",        "Transient On",          B, 0, 1, 0,        0, None, "")                           \
    X (FxTransAttack, "fx_transient_attack",    "Transient Attack",      F, -1, 1, 0.3f,    0, Bip, "")                            \
    X (FxTransSustain,"fx_transient_sustain",   "Transient Sustain",     F, -1, 1, 0,       0, Bip, "")                            \
    X (FxTransSpeed,  "fx_transient_speed",     "Transient Speed",       F, 0, 1, 0.5f,     0, Pct, "")                            \
    X (FxTransOut,    "fx_transient_out",       "Transient Output",      F, -12, 12, 0,     0, Db, "")                             \
    X (FxTransMix,    "fx_transient_mix",       "Transient Mix",         F, 0, 1, 1,        0, Pct, "")                            \
    /* Compressor */                                                                                                               \
    X (FxCompOn,      "fx_compressor_on",       "Compressor On",         B, 0, 1, 0,        0, None, "")                           \
    X (FxCompThresh,  "fx_compressor_thresh",   "Comp Threshold",        F, -60, 0, -18,    0, Db, "")                             \
    X (FxCompRatio,   "fx_compressor_ratio",    "Comp Ratio",            F, 1, 20, 3,       4, Ratio, "")                          \
    X (FxCompAttack,  "fx_compressor_attack",   "Comp Attack",           F, 0.1f, 200, 10,  10, Ms, "")                            \
    X (FxCompRelease, "fx_compressor_release",  "Comp Release",          F, 10, 1500, 150,  150, Ms, "")                           \
    X (FxCompKnee,    "fx_compressor_knee",     "Comp Knee",             F, 0, 24, 6,       0, Db, "")                             \
    X (FxCompMakeup,  "fx_compressor_makeup",   "Comp Makeup",           F, 0, 24, 0,       0, Db, "")                             \
    X (FxCompMix,     "fx_compressor_mix",      "Comp Mix",              F, 0, 1, 1,        0, Pct, "")                            \
    X (FxCompHpf,     "fx_compressor_hpf",      "Comp Sidechain HPF",    F, 20, 500, 20,    -1, Hz, "")                            \
    X (FxCompAuto,    "fx_compressor_autogain", "Comp Auto Gain",        B, 0, 1, 0,        0, None, "")                           \
    /* Squash: three-band upward + downward compression */                                                                        \
    X (FxSqOn,        "fx_squash_on",           "Squash On",             B, 0, 1, 0,        0, None, "")                           \
    X (FxSqDepth,     "fx_squash_depth",        "Squash Depth",          F, 0, 1, 0.4f,     0, Pct, "")                            \
    X (FxSqTime,      "fx_squash_time",         "Squash Time",           F, 0, 1, 0.4f,     0, Pct, "")                            \
    X (FxSqUpward,    "fx_squash_upward",       "Squash Upward",         F, 0, 1, 0.6f,     0, Pct, "")                            \
    X (FxSqOut,       "fx_squash_out",          "Squash Output",         F, -12, 12, 0,     0, Db, "")                             \
    X (FxSqMix,       "fx_squash_mix",          "Squash Mix",            F, 0, 1, 1,        0, Pct, "")                            \
    /* EQ: low cut, low shelf, two bells, high shelf, high cut */                                                                  \
    X (FxEqOn,        "fx_eq_on",               "EQ On",                 B, 0, 1, 0,        0, None, "")                           \
    X (FxEqLowCut,    "fx_eq_lowcut",           "EQ Low Cut",            F, 20, 2000, 20,   -1, Hz, "")                            \
    X (FxEqLowSlope,  "fx_eq_lowcut_slope",     "EQ Low Cut Slope",      C, 0, 1, 1,        0, None, "12 dB|24 dB")                \
    X (FxEqLowFreq,   "fx_eq_low_freq",         "EQ Low Freq",           F, 20, 1000, 120,  -1, Hz, "")                            \
    X (FxEqLowGain,   "fx_eq_low_gain",         "EQ Low Gain",           F, -18, 18, 0,     0, Db, "")                             \
    X (FxEqMidFreq,   "fx_eq_mid_freq",         "EQ Mid Freq",           F, 40, 16000, 400, -1, Hz, "")                            \
    X (FxEqMidGain,   "fx_eq_mid_gain",         "EQ Mid Gain",           F, -18, 18, 0,     0, Db, "")                             \
    X (FxEqMidQ,      "fx_eq_mid_q",            "EQ Mid Q",              F, 0.2f, 12, 1,    1, Plain, "")                          \
    X (FxEqMid2Freq,  "fx_eq_mid2_freq",        "EQ Mid 2 Freq",         F, 40, 16000, 3000,-1, Hz, "")                            \
    X (FxEqMid2Gain,  "fx_eq_mid2_gain",        "EQ Mid 2 Gain",         F, -18, 18, 0,     0, Db, "")                             \
    X (FxEqMid2Q,     "fx_eq_mid2_q",           "EQ Mid 2 Q",            F, 0.2f, 12, 1,    1, Plain, "")                          \
    X (FxEqHighFreq,  "fx_eq_high_freq",        "EQ High Freq",          F, 1000, 20000, 8000, -1, Hz, "")                         \
    X (FxEqHighGain,  "fx_eq_high_gain",        "EQ High Gain",          F, -18, 18, 0,     0, Db, "")                             \
    X (FxEqHighCut,   "fx_eq_highcut",          "EQ High Cut",           F, 200, 20000, 20000, -1, Hz, "")                         \
    X (FxEqHighSlope, "fx_eq_highcut_slope",    "EQ High Cut Slope",     C, 0, 1, 1,        0, None, "12 dB|24 dB")                \
    /* Filter: ladder or state-variable, envelope on every note / hit, LFO */                                                     \
    X (FxFiltOn,      "fx_filter_on",           "Filter On",             B, 0, 1, 0,        0, None, "")                           \
    X (FxFiltType,    "fx_filter_type",         "Filter Type",           C, 0, 4, 0,        0, None, "Ladder|Low Pass|Band Pass|High Pass|Notch") \
    X (FxFiltCutoff,  "fx_filter_cutoff",       "Filter Cutoff",         F, 20, 20000, 2000, -1, Hz, "")                           \
    X (FxFiltReso,    "fx_filter_reso",         "Filter Resonance",      F, 0, 1, 0.2f,     0, Pct, "")                            \
    X (FxFiltDrive,   "fx_filter_drive",        "Filter Drive",          F, 0, 1, 0,        0, Pct, "")                            \
    X (FxFiltEnv,     "fx_filter_env",          "Filter Env Amount",     F, -6, 6, 0,       0, Oct, "")                            \
    X (FxFiltDecay,   "fx_filter_decay",        "Filter Env Decay",      F, 0.005f, 2, 0.15f, 0.15f, Time, "")                     \
    X (FxFiltTrigger, "fx_filter_trigger",      "Filter Env Trigger",    C, 0, 1, 0,        0, None, "Notes|Audio")                \
    X (FxFiltLfoShape,"fx_filter_lfo_shape",    "Filter LFO Shape",      C, 0, 4, 0,        0, None, "Sine|Triangle|Saw|Square|Random") \
    X (FxFiltLfoSync, "fx_filter_lfo_sync",     "Filter LFO Sync",       B, 0, 1, 0,        0, None, "")                           \
    X (FxFiltLfoRate, "fx_filter_lfo_rate",     "Filter LFO Rate",       F, 0.02f, 20, 1,   -1, Hz, "")                            \
    X (FxFiltLfoNote, "fx_filter_lfo_note",     "Filter LFO Note",       C, 0, 14, 8,       0, None, FJORD_FX_NOTES)               \
    X (FxFiltLfoDepth,"fx_filter_lfo_depth",    "Filter LFO Depth",      F, 0, 4, 0,        0, Oct, "")                            \
    X (FxFiltMix,     "fx_filter_mix",          "Filter Mix",            F, 0, 1, 1,        0, Pct, "")                            \
    /* Motion: chorus, ensemble, phaser, flanger, doubler, vibrato */                                                              \
    X (FxMotOn,       "fx_motion_on",           "Motion On",             B, 0, 1, 0,        0, None, "")                           \
    X (FxMotMode,     "fx_motion_mode",         "Motion Mode",           C, 0, 5, 0,        0, None, "Chorus|Ensemble|Phaser|Flanger|Doubler|Vibrato") \
    X (FxMotSync,     "fx_motion_sync",         "Motion Sync",           B, 0, 1, 0,        0, None, "")                           \
    X (FxMotRate,     "fx_motion_rate",         "Motion Rate",           F, 0.02f, 10, 0.5f, -1, Hz, "")                           \
    X (FxMotNote,     "fx_motion_note",         "Motion Note",           C, 0, 14, 11,      0, None, FJORD_FX_NOTES)               \
    X (FxMotDepth,    "fx_motion_depth",        "Motion Depth",          F, 0, 1, 0.5f,     0, Pct, "")                            \
    X (FxMotFeedback, "fx_motion_feedback",     "Motion Feedback",       F, 0, 1, 0.2f,     0, Pct, "")                            \
    X (FxMotMix,      "fx_motion_mix",          "Motion Mix",            F, 0, 1, 0.5f,     0, Pct, "")                            \
    /* Delay: clean, tape, grains, diffuse */                                                                                      \
    X (FxDlyOn,       "fx_delay_on",            "Delay On",              B, 0, 1, 0,        0, None, "")                           \
    X (FxDlyMode,     "fx_delay_mode",          "Delay Character",       C, 0, 3, 0,        0, None, "Clean|Tape|Grains|Diffuse")  \
    X (FxDlySync,     "fx_delay_sync",          "Delay Sync",            B, 0, 1, 1,        0, None, "")                           \
    X (FxDlyNote,     "fx_delay_note",          "Delay Note",            C, 0, 14, 7,       0, None, FJORD_FX_NOTES)               \
    X (FxDlyTime,     "fx_delay_time",          "Delay Time",            F, 1, 2000, 350,   350, Ms, "")                           \
    X (FxDlyFeedback, "fx_delay_feedback",      "Delay Feedback",        F, 0, 0.95f, 0.35f, 0, Pct, "")                           \
    X (FxDlyPingPong, "fx_delay_pingpong",      "Delay Ping-Pong",       B, 0, 1, 1,        0, None, "")                           \
    X (FxDlySpread,   "fx_delay_spread",        "Delay Stereo Offset",   F, 0, 1, 0,        0, Pct, "")                            \
    X (FxDlyDamp,     "fx_delay_damp",          "Delay Damping",         F, 0, 1, 0.4f,     0, Pct, "")                            \
    X (FxDlyLowCut,   "fx_delay_lowcut",        "Delay Low Cut",         F, 20, 2000, 90,   -1, Hz, "")                            \
    X (FxDlyMod,      "fx_delay_mod",           "Delay Modulation",      F, 0, 1, 0,        0, Pct, "")                            \
    X (FxDlyDuck,     "fx_delay_duck",          "Delay Ducking",         F, 0, 1, 0,        0, Pct, "")                            \
    X (FxDlyMix,      "fx_delay_mix",           "Delay Mix",             F, 0, 1, 0.25f,    0, Pct, "")                            \
    /* Reverb: room, plate, hall, shimmer, cloud */                                                                                \
    X (FxVerbOn,      "fx_reverb_on",           "Reverb On",             B, 0, 1, 0,        0, None, "")                           \
    X (FxVerbMode,    "fx_reverb_mode",         "Reverb Space",          C, 0, 5, 2,        0, None, "Room|Plate|Hall|Shimmer|Cloud|Studio") \
    X (FxVerbSize,    "fx_reverb_size",         "Reverb Size",           F, 0, 1, 0.6f,     0, Pct, "")                            \
    X (FxVerbDecay,   "fx_reverb_decay",        "Reverb Decay",          F, 0.1f, 20, 2.5f, 2, Time, "")                           \
    X (FxVerbDamp,    "fx_reverb_damp",         "Reverb Damping",        F, 0, 1, 0.5f,     0, Pct, "")                            \
    X (FxVerbPre,     "fx_reverb_predelay",     "Reverb Pre-Delay",      F, 0, 0.25f, 0.02f, 0.05f, Time, "")                      \
    X (FxVerbLowCut,  "fx_reverb_lowcut",       "Reverb Low Cut",        F, 20, 1000, 20,   -1, Hz, "")                            \
    X (FxVerbWidth,   "fx_reverb_width",        "Reverb Width",          F, 0, 1.5f, 1,     0, Pct, "")                            \
    X (FxVerbShimmer, "fx_reverb_shimmer",      "Reverb Shimmer",        F, 0, 1, 0.4f,     0, Pct, "")                            \
    X (FxVerbMod,     "fx_reverb_mod",          "Reverb Modulation",     F, 0, 1, 0.3f,     0, Pct, "")                            \
    X (FxVerbFreeze,  "fx_reverb_freeze",       "Reverb Freeze",         B, 0, 1, 0,        0, None, "")                           \
    X (FxVerbDuck,    "fx_reverb_duck",         "Reverb Ducking",        F, 0, 1, 0,        0, Pct, "")                            \
    X (FxVerbMix,     "fx_reverb_mix",          "Reverb Mix",            F, 0, 1, 0.25f,    0, Pct, "")                            \
    /* Width: mid / side, mono below a crossover, Haas spread */                                                                   \
    X (FxWidthOn,     "fx_width_on",            "Width On",              B, 0, 1, 0,        0, None, "")                           \
    X (FxWidthAmount, "fx_width_amount",        "Stereo Width",          F, 0, 2, 1,        0, Pct, "")                            \
    X (FxWidthMono,   "fx_width_mono",          "Mono Below",            F, 20, 500, 20,    -1, Hz, "")                            \
    X (FxWidthHaas,   "fx_width_haas",          "Haas Spread",           F, 0, 30, 0,       0, Ms, "")                             \
    /* Tremolo / auto-pan */                                                                                                       \
    X (FxTremOn,      "fx_tremolo_on",          "Tremolo On",            B, 0, 1, 0,        0, None, "")                           \
    X (FxTremMode,    "fx_tremolo_mode",        "Tremolo Mode",          C, 0, 2, 0,        0, None, "Tremolo|Auto-Pan|Harmonic")  \
    X (FxTremShape,   "fx_tremolo_shape",       "Tremolo Shape",         C, 0, 4, 0,        0, None, "Sine|Triangle|Square|Saw|Random") \
    X (FxTremSync,    "fx_tremolo_sync",        "Tremolo Sync",          B, 0, 1, 1,        0, None, "")                           \
    X (FxTremRate,    "fx_tremolo_rate",        "Tremolo Rate",          F, 0.1f, 20, 4,    -1, Hz, "")                            \
    X (FxTremNote,    "fx_tremolo_note",        "Tremolo Note",          C, 0, 14, 5,       0, None, FJORD_FX_NOTES)               \
    X (FxTremDepth,   "fx_tremolo_depth",       "Tremolo Depth",         F, 0, 1, 0.5f,     0, Pct, "")                            \
    X (FxTremStereo,  "fx_tremolo_stereo",      "Tremolo Stereo Phase",  F, 0, 180, 0,      0, Deg, "")                            \
    X (FxTremSmooth,  "fx_tremolo_smooth",      "Tremolo Smoothing",     F, 0, 1, 0.3f,     0, Pct, "")                            \
    X (FxTremMix,     "fx_tremolo_mix",         "Tremolo Mix",           F, 0, 1, 1,        0, Pct, "")                            \
    /* Frequency shifter / ring modulator */                                                                                       \
    X (FxShiftOn,     "fx_shifter_on",          "Shifter On",            B, 0, 1, 0,        0, None, "")                           \
    X (FxShiftMode,   "fx_shifter_mode",        "Shifter Mode",          C, 0, 1, 0,        0, None, "Shift|Ring")                 \
    X (FxShiftAmount, "fx_shifter_shift",       "Frequency Shift",       F, -2000, 2000, 5, 100, Hz, "")                           \
    X (FxShiftFeedback,"fx_shifter_feedback",   "Shifter Feedback",      F, 0, 0.9f, 0,     0, Pct, "")                            \
    X (FxShiftSpread, "fx_shifter_spread",      "Shifter Stereo",        F, 0, 1, 0,        0, Pct, "")                            \
    X (FxShiftMix,    "fx_shifter_mix",         "Shifter Mix",           F, 0, 1, 0.5f,     0, Pct, "")                            \
    /* Exciter */                                                                                                                  \
    X (FxExcOn,       "fx_exciter_on",          "Exciter On",            B, 0, 1, 0,        0, None, "")                           \
    X (FxExcFreq,     "fx_exciter_freq",        "Exciter Frequency",     F, 800, 12000, 3000, -1, Hz, "")                          \
    X (FxExcDrive,    "fx_exciter_drive",       "Exciter Drive",         F, 0, 1, 0.4f,     0, Pct, "")                            \
    X (FxExcColor,    "fx_exciter_color",       "Exciter Colour",        F, -1, 1, 0,       0, Bip, "")                            \
    X (FxExcAmount,   "fx_exciter_amount",      "Exciter Amount",        F, 0, 1, 0.3f,     0, Pct, "")                            \
    /* Harmonizer: two pitched voices */                                                                                           \
    X (FxHarmOn,      "fx_harmonizer_on",       "Harmonizer On",         B, 0, 1, 0,        0, None, "")                           \
    X (FxHarmPitch1,  "fx_harmonizer_pitch1",   "Harmonizer Voice 1",    I, -24, 24, 12,    0, Semi, "")                           \
    X (FxHarmLevel1,  "fx_harmonizer_level1",   "Harmonizer Level 1",    F, 0, 1, 0.7f,     0, Pct, "")                            \
    X (FxHarmPitch2,  "fx_harmonizer_pitch2",   "Harmonizer Voice 2",    I, -24, 24, 7,     0, Semi, "")                           \
    X (FxHarmLevel2,  "fx_harmonizer_level2",   "Harmonizer Level 2",    F, 0, 1, 0,        0, Pct, "")                            \
    X (FxHarmDetune,  "fx_harmonizer_detune",   "Harmonizer Detune",     F, 0, 50, 6,       0, Cents, "")                          \
    X (FxHarmDelay,   "fx_harmonizer_delay",    "Harmonizer Delay",      F, 0, 120, 12,     0, Ms, "")                             \
    X (FxHarmFeedback,"fx_harmonizer_feedback", "Harmonizer Feedback",   F, 0, 0.8f, 0,     0, Pct, "")                            \
    X (FxHarmTone,    "fx_harmonizer_tone",     "Harmonizer Tone",       F, -1, 1, 0,       0, Bip, "")                            \
    X (FxHarmMix,     "fx_harmonizer_mix",      "Harmonizer Mix",        F, 0, 1, 0.35f,    0, Pct, "")                            \
    /* Clipper: always last */                                                                                                     \
    X (FxOutOn,       "fx_out_on",              "Clipper On",            B, 0, 1, 1,        0, None, "")                           \
    X (FxOutMode,     "fx_out_mode",            "Clipper Curve",         C, 0, 2, 0,        0, None, "Soft|Hard|Tape")             \
    X (FxOutDrive,    "fx_out_drive",           "Clipper Drive",         F, 0, 24, 0,       0, Db, "")                             \
    X (FxOutCeiling,  "fx_out_ceiling",         "Clipper Ceiling",       F, -12, 0, -0.3f,  0, Db, "")

namespace fjord::base::fx
{
    // Index of every effect parameter inside the block (the plugin table holds the block in this order).
    enum Param : int { FJORD_FX_PARAMS (FJORD_PARAM_ENUM) NumParams };

    const std::array<params::Spec, NumParams>& specs();
    inline const params::Spec& spec (Param p) { return specs()[(size_t) p]; }
    inline juce::String paramId (Param p) { return spec (p).paramId; }

    // The effect modules. The order is stored nowhere (the effect order uses the string IDs), but keep appending.
    enum class Module { Saturation, Lofi, Transient, Compressor, Squash, Eq, Filter, Motion, Delay, Reverb, Width,
                        Tremolo, Shifter, Exciter, Harmonizer, Out, Count };
    constexpr int kNumModules = (int) Module::Count;
    constexpr int kNumChain = kNumModules - 1;   // the clipper is always last and not part of the order
    constexpr int kOut = (int) Module::Out;

    struct ModuleInfo
    {
        const char* id;            // "reverb" – parameter prefix, preset file name, order string
        const char* name;          // "Reverb"
        const char* tile;          // short name on the effect tiles
        const char* description;   // one line below the title
        Param first, last;         // its parameter block (first = the on switch)
    };

    const ModuleInfo& info (int module);
    inline const ModuleInfo& info (Module m) { return info ((int) m); }
    inline Param onParam (int module) { return info (module).first; }
    int moduleOf (Param);
    int moduleFromId (const juce::String& id);   // -1 if unknown
    int moduleOfParamId (const juce::String& paramId);   // "fx_reverb_size" -> Reverb, -1 for other parameters

    // Plain values of the effect block (a view into the plugin's values, or an own array).
    struct Values
    {
        const float* v = nullptr;
        float operator[] (Param p) const { return v[(int) p]; }
        int i (Param p) const { return (int) std::lround (v[(int) p]); }
        bool b (Param p) const { return v[(int) p] > 0.5f; }
    };

    // Default values of the whole block (tests, offline rendering, graphs)
    const std::array<float, NumParams>& defaults();

    // Length of a note value from FJORD_FX_NOTES in quarter notes
    double noteBeats (int index);

    // Effect order: the chain modules (all but the clipper) as ids, e.g. "saturation,eq,compressor,...".
    using Order = std::array<int, kNumChain>;
    Order defaultOrder();
    juce::String orderToString (const Order&);
    // Unknown, duplicate or missing entries are fixed: missing modules are inserted at their default position.
    Order orderFromString (const juce::String&, const Order& fallback = defaultOrder());
}
