# FM1 Bass Station TEST1 — monophonic core sound probe

## Purpose

TEST1 establishes whether the legacy Novation Bass Station architecture is a good native fit for the FM1 before spending time on full preset recovery and detailed desktop comparison.

Unlike the previous V-Station project, Bass Station is deliberately kept monophonic. No polyphony scheduler or per-note DSP pool is present.

## Implemented core

- two 44.1 kHz oscillators
- saw + variable pulse waves
- 16'/8'/4'/2' ranges
- OSC2 semitone and fine detune
- OSC1-master oscillator sync
- manual / ENV2 / fixed PWM-LFO pulse width modulation
- oscillator/noise mixer
- nonlinear resonant low-pass filter with 12 dB / 24 dB modes
- amp ADSR and modulation ADSR
- triangle / falling-saw / S&H LFO with delay
- ENV2 and LFO modulation routes for pitch/filter
- glide
- MIDI pitch bend, CC1 mod wheel, CC64 sustain and all-notes-off
- last-note monophonic priority with held-note return
- physical FM1 UI and performance counters
- diagnostic presets: INIT, BASS, SYNC, PWM, REZ

## Validation

- target build passes FM1 image/RAM/register audits
- no software-double helpers in target
- DSP ASan/UBSan/float-cast-overflow stress passes at extreme resonance, modulation, sync and PWM settings
- host UI/MIDI smoke passes page navigation, preset wrap, local keyboard audio, USB MIDI, sustain, mod wheel and sync toggle
- host simulator reports roughly 0.4% real-time render cost on the validation machine (directional only; not an FM1 CPU measurement)

## Target resources

- application: 86,572 / 581,564 bytes
- main RAM: 18,760 / 98,304 bytes
- large-buffer pool: 69,124 / 344,064 bytes

The very small footprint leaves ample headroom for fidelity work, preset recovery, USB editor support and diagnostics.

## Intentional TEST1 limits

- no recovered 100-preset legacy software bank yet
- no claim of bit-exact Novation plugin audio
- MIDI-clock LFO sync and some hardware-era controller routing are not yet exposed
- filter/oscillator character will be refined after real FM1 listening

## Files

- `FM1-BassStation-TEST1.fwsc` — preferred application update
- `FM1-BassStation-TEST1-M-Flasher.bin` — complete 1 MiB image using the pinned reference backup
- self-contained source/test kit — SDK/build scripts + compressed JieLi toolchain archive
