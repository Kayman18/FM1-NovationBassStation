# FM1 Bass Station TEST2 — fidelity/behavior pass

## Why this build exists

FM1 hardware testing of TEST1 showed very large CPU headroom (about 7% average / 12% peak, 752 us maximum work, zero deadlines in the reported run). TEST2 therefore spends resources on behavior and filter quality rather than optimization.

## Changes from TEST1

- Added the original Bass Station envelope trigger choices: **AUTOGLIDE / SINGLE / MULTI**.
  - AUTOGLIDE keeps legato pitch transitions gliding without restarting the envelopes.
  - SINGLE changes held-note pitch without restarting the envelopes.
  - MULTI restarts the envelopes for every newly played key.
- Added independent **Envelope 1 (amp) and Envelope 2 velocity sensitivity**.
- Held-note return now restores the selected held note's velocity as well as its pitch.
- Reworked pulse width control to follow the hardware control semantics:
  - MANUAL: the PW control moves from 50% square toward a narrow pulse.
  - ENV2 / PWM-LFO: 50% is the centre point and the PW control becomes bipolar modulation depth.
- Reworked filter modulation to one bipolar **MOD DEPTH** plus selectable **ENV2 / LFO** source.
- Added documented Bass Station filter-range behavior:
  - 12 dB mode is restricted to about 4.5 kHz and has no keyboard tracking.
  - 24 dB mode extends to about 10 kHz and uses gentle keyboard tracking.
- The nonlinear filter now runs internally at **88.2 kHz (2x)** while FM1 audio remains 44.1 kHz.
- Envelope control ranges now follow the hardware ranges more closely: attack 0.5 ms..5 s, decay/release 3 ms..10 s.
- Added a dedicated OSC MODULATION page so oscillator ENV/LFO pitch depths and OSC2 fine detune are directly editable.
- Added a TRIGGER / VELOCITY page.
- Added legacy Bass Station Rack MIDI automation CC handling:
  - CC105 cutoff, CC106 resonance, CC107 filter modulation
  - CC108..112 Envelope 1 A/D/S/R/velocity
  - CC114..118 Envelope 2 A/D/S/R/velocity
- MIDI Program Change 0..4 loads the current diagnostic INIT/BASS/SYNC/PWM/REZ bank.

## Controls

- HOME — OSC1
- EDIT — toggles OSC2 / OSC MODULATION
- SEL — MIXER
- FX — FILTER
- ENV — cycles AMP ENV / MOD ENV / TRIGGER+VELOCITY
- LFO — LFO
- GLO — GLOBAL
- ARP — 12/24 dB filter slope
- SEQ — oscillator sync
- PLAY — PWM source
- PRESET encoder — INIT / BASS / SYNC / PWM / REZ
- SELECT encoder — cycles all pages, including PERFORMANCE

## Validation

- DSP ASan/UBSan/float-cast-overflow stress: passed
- Trigger-mode / held-note-return tests: passed
- Programmable velocity-response test: passed
- 12/24 dB, resonance, sync, PWM and modulation extreme stress: passed
- Host UI navigation across 11 pages: passed
- Host local keyboard / USB MIDI / sustain / CC automation smoke: passed
- Host simulator render cost: about 0.5% of real time on the validation machine (directional only)
- Target build audit: passed; no software-double helpers

## Target resources

- Application: 89,944 / 581,564 bytes
- Main RAM: 18,888 / 98,304 bytes
- Large-buffer pool: 69,124 / 344,064 bytes

## Still intentionally unfinished

- recovered legacy 100-program software bank
- exact desktop-plugin audio parity
- MIDI-clock LFO sync
- aftertouch/breath-controller routings
- final oscillator/filter character calibration against the original plug-in/hardware
- USB web editor / JSON preset manager

## Suggested hardware test

1. Compare BASS, SYNC, PWM and REZ to TEST1 at modest volume.
2. On FILTER compare 12 dB and 24 dB; 12 dB should be darker and stop opening around the documented TB-303-like top end.
3. On TRIGGER / VELOCITY compare MULTI, SINGLE and AUTOGLIDE while overlapping notes.
4. Set some glide and verify AUTOGLIDE moves smoothly between overlapping notes without retriggering the envelope.
5. Raise AMP VEL and MOD VEL and compare soft/hard playing.
6. On PWM use PLAY to switch MANUAL / ENV2 / PWM LFO and try the PW controls on OSC1/OSC2.
7. Check PERFORMANCE again; the 2x filter should still leave very large deadline margin.
