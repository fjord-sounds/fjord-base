# Effects

Every fjord-sounds plugin has the same sixteen effects on its FX page, and every effect is also a plugin of its own
("Fjord Reverb" …). The code lives in `source/fjord_base/fx/`; plugins only include the parameter block and call the
chain. This page lists the effects, their parameters and how the pieces fit together.

## The chain

- **Order**: fifteen effects can be reordered (drag the tiles); the clipper (*Out*) is always last. The order is stored
  as a string of effect ids (`fxOrder`, e.g. `saturation,transient,filter,…`), in the project state and in presets.
  Unknown or missing ids are repaired when loading (a missing effect goes behind the one that precedes it in the
  default order).
- **Parameters**: `FJORD_FX_PARAMS(X)` in `FxParams.h` is an X-macro block that every plugin pastes into its own
  parameter table. IDs are `fx_<effect>_<param>`, so effects can be automated and modulated like any other parameter,
  and presets of any plugin or effect plugin use the same ids. Rules for changing the block are in
  [compatibility-rules.md](compatibility-rules.md).
- **Processing**: `fx::Chain::process(buffer, values, order, context, meters)` runs the enabled effects in order.
  `Context` carries tempo, song position, transport state and note/hit positions (the filter envelope restarts on
  them; without notes it follows onsets in the audio). Effects allocate in `prepare`, never while processing, and
  smooth their parameters.
- **Single effects**: `Chain::processSingle(module, …)` processes one effect; the effect plugins use it.
- **Presets**: every effect has at least 40 factory presets (`presets/fx/<effect>.fjordmod`) and there are 35 whole
  chains (`presets/chains/*.fjordfx`). See [presets.md](presets.md).

| Effect | id | Plugin code | Presets | What it does |
|---|---|---|---|---|
| **Saturation** | `saturation` | `Fsat` | 46 | Six transfer curves with bias, tilt tone, oversampling and auto gain |
| **Lo-Fi** | `lofi` | `Flfi` | 46 | Tape wow and flutter, age, hiss, bit depth and sample-rate reduction |
| **Transient** | `transient` | `Ftrs` | 41 | More or less attack and sustain, independent of level |
| **Compressor** | `compressor` | `Fcmp` | 46 | Soft-knee compressor with parallel mix, sidechain filter and auto gain |
| **Squash** | `squash` | `Fsqh` | 41 | Three-band upward and downward compression - quiet details come forward |
| **EQ** | `eq` | `Fequ` | 49 | Low cut, shelves, two bells, high cut  -  drag the points, wheel = Q |
| **Filter** | `filter` | `Fflt` | 41 | Ladder or state-variable filter with an envelope per note and an LFO |
| **Motion** | `motion` | `Fmot` | 81 | Chorus, ensemble, phaser, flanger, a doubled take or vibrato |
| **Delay** | `delay` | `Fdly` | 42 | Clean, tape, grain or diffuse echoes, tempo synced, with ducking |
| **Reverb** | `reverb` | `Frvb` | 44 | Room, plate, hall, shimmer (octave-up tail) or cloud, with freeze |
| **Width** | `width` | `Fwdt` | 41 | Stereo width, mono below the crossover, Haas spread |
| **Tremolo** | `tremolo` | `Ftrm` | 41 | Tremolo, auto-pan or harmonic tremolo, tempo synced |
| **Frequency Shifter** | `shifter` | `Fshf` | 41 | Shifts every frequency by the same amount - from slow beating to bells |
| **Exciter** | `exciter` | `Fexc` | 41 | Adds harmonics above a frequency for presence and air |
| **Harmonizer** | `harmonizer` | `Fhrm` | 41 | Two pitched voices with detune, delay and feedback |
| **Clipper** | `out` | `Fclp` | 41 | Always last: soft, hard or tape clipping up to the ceiling |

Plugin names are "Fjord " plus the effect name (the clipper is "Fjord Clipper", the shifter "Fjord Frequency Shifter"); manufacturer code `Fjrd`, AU type `aufx`.

## Parameters

Ranges and defaults come straight from `FxParams.h`. Every effect also has an `on` switch (the effect plugin treats it as bypass).

### Saturation

Six transfer curves with bias, tilt tone, oversampling and auto gain.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_saturation_on` | Saturation On | switch | off / on | 0 |
| `fx_saturation_mode` | Saturation Curve | choice | Tape · Tube · Diode · Fold · Hard · Rectify | 0 |
| `fx_saturation_drive` | Saturation Drive | float | 0 … 1 | 0.3 |
| `fx_saturation_bias` | Saturation Bias | float | -1 … 1 | 0 |
| `fx_saturation_tone` | Saturation Tone | float | -1 … 1 | 0 |
| `fx_saturation_os` | Oversampling | choice | 1x · 2x · 4x | 1 |
| `fx_saturation_out` | Saturation Output | float | -24 … 12 | 0 |
| `fx_saturation_mix` | Saturation Mix | float | 0 … 1 | 1 |
| `fx_saturation_autogain` | Saturation Auto Gain | switch | off / on | 0 |

### Lo-Fi

Tape wow and flutter, age, hiss, bit depth and sample-rate reduction.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_lofi_on` | Lo-Fi On | switch | off / on | 0 |
| `fx_lofi_wow` | Lo-Fi Wow | float | 0 … 1 | 0.3 |
| `fx_lofi_flutter` | Lo-Fi Flutter | float | 0 … 1 | 0.2 |
| `fx_lofi_age` | Lo-Fi Age | float | 0 … 1 | 0.3 |
| `fx_lofi_noise` | Lo-Fi Hiss | float | 0 … 1 | 0.1 |
| `fx_lofi_bits` | Lo-Fi Bits | float | 1 … 16 | 16 |
| `fx_lofi_rate` | Lo-Fi Downsample | float | 1 … 40 | 1 |
| `fx_lofi_jitter` | Lo-Fi Jitter | float | 0 … 1 | 0 |
| `fx_lofi_mix` | Lo-Fi Mix | float | 0 … 1 | 1 |

### Transient

More or less attack and sustain, independent of level.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_transient_on` | Transient On | switch | off / on | 0 |
| `fx_transient_attack` | Transient Attack | float | -1 … 1 | 0.3 |
| `fx_transient_sustain` | Transient Sustain | float | -1 … 1 | 0 |
| `fx_transient_speed` | Transient Speed | float | 0 … 1 | 0.5 |
| `fx_transient_out` | Transient Output | float | -12 … 12 | 0 |
| `fx_transient_mix` | Transient Mix | float | 0 … 1 | 1 |

### Compressor

Soft-knee compressor with parallel mix, sidechain filter and auto gain.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_compressor_on` | Compressor On | switch | off / on | 0 |
| `fx_compressor_thresh` | Comp Threshold | float | -60 … 0 | -18 |
| `fx_compressor_ratio` | Comp Ratio | float | 1 … 20 | 3 |
| `fx_compressor_attack` | Comp Attack | float | 0.1 … 200 | 10 |
| `fx_compressor_release` | Comp Release | float | 10 … 1500 | 150 |
| `fx_compressor_knee` | Comp Knee | float | 0 … 24 | 6 |
| `fx_compressor_makeup` | Comp Makeup | float | 0 … 24 | 0 |
| `fx_compressor_mix` | Comp Mix | float | 0 … 1 | 1 |
| `fx_compressor_hpf` | Comp Sidechain HPF | float | 20 … 500 | 20 |
| `fx_compressor_autogain` | Comp Auto Gain | switch | off / on | 0 |

### Squash

Three-band upward and downward compression - quiet details come forward.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_squash_on` | Squash On | switch | off / on | 0 |
| `fx_squash_depth` | Squash Depth | float | 0 … 1 | 0.4 |
| `fx_squash_time` | Squash Time | float | 0 … 1 | 0.4 |
| `fx_squash_upward` | Squash Upward | float | 0 … 1 | 0.6 |
| `fx_squash_out` | Squash Output | float | -12 … 12 | 0 |
| `fx_squash_mix` | Squash Mix | float | 0 … 1 | 1 |

### EQ

Low cut, shelves, two bells, high cut  -  drag the points, wheel = Q.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_eq_on` | EQ On | switch | off / on | 0 |
| `fx_eq_lowcut` | EQ Low Cut | float | 20 … 2000 | 20 |
| `fx_eq_lowcut_slope` | EQ Low Cut Slope | choice | 12 dB · 24 dB | 1 |
| `fx_eq_low_freq` | EQ Low Freq | float | 20 … 1000 | 120 |
| `fx_eq_low_gain` | EQ Low Gain | float | -18 … 18 | 0 |
| `fx_eq_mid_freq` | EQ Mid Freq | float | 40 … 16000 | 400 |
| `fx_eq_mid_gain` | EQ Mid Gain | float | -18 … 18 | 0 |
| `fx_eq_mid_q` | EQ Mid Q | float | 0.2 … 12 | 1 |
| `fx_eq_mid2_freq` | EQ Mid 2 Freq | float | 40 … 16000 | 3000 |
| `fx_eq_mid2_gain` | EQ Mid 2 Gain | float | -18 … 18 | 0 |
| `fx_eq_mid2_q` | EQ Mid 2 Q | float | 0.2 … 12 | 1 |
| `fx_eq_high_freq` | EQ High Freq | float | 1000 … 20000 | 8000 |
| `fx_eq_high_gain` | EQ High Gain | float | -18 … 18 | 0 |
| `fx_eq_highcut` | EQ High Cut | float | 200 … 20000 | 20000 |
| `fx_eq_highcut_slope` | EQ High Cut Slope | choice | 12 dB · 24 dB | 1 |

### Filter

Ladder or state-variable filter with an envelope per note and an LFO.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_filter_on` | Filter On | switch | off / on | 0 |
| `fx_filter_type` | Filter Type | choice | Ladder · Low Pass · Band Pass · High Pass · Notch | 0 |
| `fx_filter_cutoff` | Filter Cutoff | float | 20 … 20000 | 2000 |
| `fx_filter_reso` | Filter Resonance | float | 0 … 1 | 0.2 |
| `fx_filter_drive` | Filter Drive | float | 0 … 1 | 0 |
| `fx_filter_env` | Filter Env Amount | float | -6 … 6 | 0 |
| `fx_filter_decay` | Filter Env Decay | float | 0.005 … 2 | 0.15 |
| `fx_filter_trigger` | Filter Env Trigger | choice | Notes · Audio | 0 |
| `fx_filter_lfo_shape` | Filter LFO Shape | choice | Sine · Triangle · Saw · Square · Random | 0 |
| `fx_filter_lfo_sync` | Filter LFO Sync | switch | off / on | 0 |
| `fx_filter_lfo_rate` | Filter LFO Rate | float | 0.02 … 20 | 1 |
| `fx_filter_lfo_note` | Filter LFO Note | choice | 1/32 … 2 Bars (15 note values) | 8 |
| `fx_filter_lfo_depth` | Filter LFO Depth | float | 0 … 4 | 0 |
| `fx_filter_mix` | Filter Mix | float | 0 … 1 | 1 |

### Motion

Chorus, ensemble, phaser, flanger, a doubled take or vibrato.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_motion_on` | Motion On | switch | off / on | 0 |
| `fx_motion_mode` | Motion Mode | choice | Chorus · Ensemble · Phaser · Flanger · Doubler · Vibrato | 0 |
| `fx_motion_sync` | Motion Sync | switch | off / on | 0 |
| `fx_motion_rate` | Motion Rate | float | 0.02 … 10 | 0.5 |
| `fx_motion_note` | Motion Note | choice | 1/32 … 2 Bars (15 note values) | 11 |
| `fx_motion_depth` | Motion Depth | float | 0 … 1 | 0.5 |
| `fx_motion_feedback` | Motion Feedback | float | 0 … 1 | 0.2 |
| `fx_motion_mix` | Motion Mix | float | 0 … 1 | 0.5 |

### Delay

Clean, tape, grain or diffuse echoes, tempo synced, with ducking.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_delay_on` | Delay On | switch | off / on | 0 |
| `fx_delay_mode` | Delay Character | choice | Clean · Tape · Grains · Diffuse | 0 |
| `fx_delay_sync` | Delay Sync | switch | off / on | 1 |
| `fx_delay_note` | Delay Note | choice | 1/32 … 2 Bars (15 note values) | 7 |
| `fx_delay_time` | Delay Time | float | 1 … 2000 | 350 |
| `fx_delay_feedback` | Delay Feedback | float | 0 … 0.95 | 0.35 |
| `fx_delay_pingpong` | Delay Ping-Pong | switch | off / on | 1 |
| `fx_delay_spread` | Delay Stereo Offset | float | 0 … 1 | 0 |
| `fx_delay_damp` | Delay Damping | float | 0 … 1 | 0.4 |
| `fx_delay_lowcut` | Delay Low Cut | float | 20 … 2000 | 90 |
| `fx_delay_mod` | Delay Modulation | float | 0 … 1 | 0 |
| `fx_delay_duck` | Delay Ducking | float | 0 … 1 | 0 |
| `fx_delay_mix` | Delay Mix | float | 0 … 1 | 0.25 |

### Reverb

Room, plate, hall, shimmer (octave-up tail) or cloud, with freeze.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_reverb_on` | Reverb On | switch | off / on | 0 |
| `fx_reverb_mode` | Reverb Space | choice | Room · Plate · Hall · Shimmer · Cloud · Studio | 2 |
| `fx_reverb_size` | Reverb Size | float | 0 … 1 | 0.6 |
| `fx_reverb_decay` | Reverb Decay | float | 0.1 … 20 | 2.5 |
| `fx_reverb_damp` | Reverb Damping | float | 0 … 1 | 0.5 |
| `fx_reverb_predelay` | Reverb Pre-Delay | float | 0 … 0.25 | 0.02 |
| `fx_reverb_lowcut` | Reverb Low Cut | float | 20 … 1000 | 20 |
| `fx_reverb_width` | Reverb Width | float | 0 … 1.5 | 1 |
| `fx_reverb_shimmer` | Reverb Shimmer | float | 0 … 1 | 0.4 |
| `fx_reverb_mod` | Reverb Modulation | float | 0 … 1 | 0.3 |
| `fx_reverb_freeze` | Reverb Freeze | switch | off / on | 0 |
| `fx_reverb_duck` | Reverb Ducking | float | 0 … 1 | 0 |
| `fx_reverb_mix` | Reverb Mix | float | 0 … 1 | 0.25 |

### Width

Stereo width, mono below the crossover, Haas spread.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_width_on` | Width On | switch | off / on | 0 |
| `fx_width_amount` | Stereo Width | float | 0 … 2 | 1 |
| `fx_width_mono` | Mono Below | float | 20 … 500 | 20 |
| `fx_width_haas` | Haas Spread | float | 0 … 30 | 0 |

### Tremolo

Tremolo, auto-pan or harmonic tremolo, tempo synced.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_tremolo_on` | Tremolo On | switch | off / on | 0 |
| `fx_tremolo_mode` | Tremolo Mode | choice | Tremolo · Auto-Pan · Harmonic | 0 |
| `fx_tremolo_shape` | Tremolo Shape | choice | Sine · Triangle · Square · Saw · Random | 0 |
| `fx_tremolo_sync` | Tremolo Sync | switch | off / on | 1 |
| `fx_tremolo_rate` | Tremolo Rate | float | 0.1 … 20 | 4 |
| `fx_tremolo_note` | Tremolo Note | choice | 1/32 … 2 Bars (15 note values) | 5 |
| `fx_tremolo_depth` | Tremolo Depth | float | 0 … 1 | 0.5 |
| `fx_tremolo_stereo` | Tremolo Stereo Phase | float | 0 … 180 | 0 |
| `fx_tremolo_smooth` | Tremolo Smoothing | float | 0 … 1 | 0.3 |
| `fx_tremolo_mix` | Tremolo Mix | float | 0 … 1 | 1 |

### Frequency Shifter

Shifts every frequency by the same amount - from slow beating to bells.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_shifter_on` | Shifter On | switch | off / on | 0 |
| `fx_shifter_mode` | Shifter Mode | choice | Shift · Ring | 0 |
| `fx_shifter_shift` | Frequency Shift | float | -2000 … 2000 | 5 |
| `fx_shifter_feedback` | Shifter Feedback | float | 0 … 0.9 | 0 |
| `fx_shifter_spread` | Shifter Stereo | float | 0 … 1 | 0 |
| `fx_shifter_mix` | Shifter Mix | float | 0 … 1 | 0.5 |

### Exciter

Adds harmonics above a frequency for presence and air.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_exciter_on` | Exciter On | switch | off / on | 0 |
| `fx_exciter_freq` | Exciter Frequency | float | 800 … 12000 | 3000 |
| `fx_exciter_drive` | Exciter Drive | float | 0 … 1 | 0.4 |
| `fx_exciter_color` | Exciter Colour | float | -1 … 1 | 0 |
| `fx_exciter_amount` | Exciter Amount | float | 0 … 1 | 0.3 |

### Harmonizer

Two pitched voices with detune, delay and feedback.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_harmonizer_on` | Harmonizer On | switch | off / on | 0 |
| `fx_harmonizer_pitch1` | Harmonizer Voice 1 | int | -24 … 24 | 12 |
| `fx_harmonizer_level1` | Harmonizer Level 1 | float | 0 … 1 | 0.7 |
| `fx_harmonizer_pitch2` | Harmonizer Voice 2 | int | -24 … 24 | 7 |
| `fx_harmonizer_level2` | Harmonizer Level 2 | float | 0 … 1 | 0 |
| `fx_harmonizer_detune` | Harmonizer Detune | float | 0 … 50 | 6 |
| `fx_harmonizer_delay` | Harmonizer Delay | float | 0 … 120 | 12 |
| `fx_harmonizer_feedback` | Harmonizer Feedback | float | 0 … 0.8 | 0 |
| `fx_harmonizer_tone` | Harmonizer Tone | float | -1 … 1 | 0 |
| `fx_harmonizer_mix` | Harmonizer Mix | float | 0 … 1 | 0.35 |

### Clipper

Always last: soft, hard or tape clipping up to the ceiling.

| Parameter id | Name | Kind | Range | Default |
|---|---|---|---|---|
| `fx_out_on` | Clipper On | switch | off / on | 1 |
| `fx_out_mode` | Clipper Curve | choice | Soft · Hard · Tape | 0 |
| `fx_out_drive` | Clipper Drive | float | 0 … 24 | 0 |
| `fx_out_ceiling` | Clipper Ceiling | float | -12 … 0 | -0.3 |
