# FM1 Bass Station TEST3 — MIDI clock + Aftertouch/Breath performance control

## Purpose

TEST2 established that the monophonic Bass Station engine has very large FM1 CPU headroom. TEST3 keeps the full-rate 44.1 kHz oscillator path and 2x nonlinear filter, and completes more of the original Rack performance-controller behavior rather than optimizing DSP.

## Changes from TEST2

- Added MIDI-clock-synchronised LFO mode with the documented Bass Station Rack divisions:
  - free-running / OFF
  - 1/32 triplet, 1/32
  - 1/16 triplet, 1/16
  - 1/8 triplet, dotted 1/16, 1/8
  - 1/4 triplet, dotted 1/8, 1/4
  - 1/2 triplet, dotted 1/4, 1/2
  - dotted 1/2, whole note
- MIDI Start/Continue restarts/enables the clocked LFO; MIDI Stop freezes clocked advancement until clock resumes.
- MIDI-clock period is measured and smoothed from incoming clock timing so the LFO runs continuously between clock ticks instead of becoming a 24-step staircase.
- Added channel Aftertouch routing for:
  - pitch modulation depth (-7..+7 utility scale)
  - filter modulation depth (-7..+7 utility scale)
  - envelope/VCA bias (0..15 utility scale)
- Added Breath Controller (CC2) routing for the same three destinations with independent stored depths.
- Added dedicated AFTERTOUCH and BREATH FM1 pages.
- GLOBAL now exposes bend range, wheel pitch depth, wheel filter depth and volume.
- LFO page now exposes the MIDI-clock division directly.
- GLO cycles GLOBAL -> AFTERTOUCH -> BREATH.

The pressure/breath depth numbers preserve the original utility ranges. Their exact analogue scaling remains a calibration boundary; TEST3 implements conservative musical mappings while preserving sign/range and independent patch storage.

## Controls

- HOME — OSC1
- EDIT — toggles OSC2 / OSC MODULATION
- SEL — MIXER
- FX — FILTER
- ENV — cycles AMP ENV / MOD ENV / TRIGGER+VELOCITY
- LFO — LFO + MIDI CLOCK
- GLO — cycles GLOBAL / AFTERTOUCH / BREATH
- ARP — 12/24 dB filter slope
- SEQ — oscillator sync
- PLAY — PWM source
- PRESET encoder — INIT / BASS / SYNC / PWM / REZ diagnostic bank
- SELECT encoder — cycles all pages, including PERFORMANCE

## Validation

- DSP ASan/UBSan/float-cast-overflow stress: passed
- Original TEST2 trigger/velocity/filter/sync/PWM stress: passed
- Aftertouch/Breath routing stress: passed
- MIDI-clock period acquisition and clocked LFO advancement: passed
- Host UI navigation across 13 pages: passed
- Host local keyboard / USB MIDI / sustain / pressure / breath / CC automation smoke: passed
- Host simulator render cost: about 0.5% of real time on the validation machine (directional only)
- Target build audit: passed; no software-double helpers

## Target resources

- Application: 91,252 / 581,564 bytes
- Main RAM: 18,968 / 98,304 bytes
- Large-buffer pool: 69,124 / 344,064 bytes

## Still intentionally unfinished

- recovered legacy software factory bank
- exact desktop-plugin audio parity
- final oscillator/filter character calibration against the original plug-in/hardware
- exact analogue scaling of Aftertouch/Breath utility depths
- USB web editor / JSON preset manager

## Suggested hardware test

1. Confirm TEST2 sound and trigger behavior are unchanged.
2. Set LFO -> MIDI CLOCK to 1/4. Send MIDI clock and transport from a DAW; pitch/filter modulation should lock to quarter notes.
3. Try 1/8, dotted 1/8 and 1/16 triplet divisions.
4. On AFTERTOUCH set modest positive PITCH/FILTER depths and verify channel pressure changes the sound.
5. On BREATH set a filter depth and send CC2 from a controller/DAW.
6. Try EG BIAS cautiously; high values can open the VCA strongly.
7. Check PERFORMANCE; TEST3 should remain far below the FM1 audio deadline.
