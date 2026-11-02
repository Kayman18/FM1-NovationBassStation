# FM1 Bass Station RC1

Release Candidate 1 promotes the TEST21 codebase with the requested hardware-control and screen-layout cleanup.

## RC1 changes

- SELECT encoder is a permanent live Filter Cutoff control everywhere except the SEL preset-browser page.
- ALGO encoder is a permanent live Filter Resonance control everywhere except the SEL preset-browser page.
- On the SEL preset-browser page, SELECT/PRESET browse patches and ALGO switches Factory/User bank.
- PRESET keeps preset selection in the synth and step/panel selection in the sequencer.
- Because SELECT no longer navigates pages, EDIT now cycles OSC1 -> OSC2 -> OSC MOD -> MIX, and GLO cycles GLOBAL -> AFTERTOUCH -> BREATH -> PERFORMANCE.
- All normal parameter pages use a clean 2x2 four-control layout across the full 240x240 display; the old lower information/footer panel is removed.
- 303 Generate and Mutate use the same full 2x2 control layout; editor pages retain their specialized grid and no longer show the old lower help panel.
- Preset browser clears and renders both physical display bands cleanly, with ALGO shown as the bank selector.
- Splash hold is doubled from 400 ms to 800 ms, and the splash is marked RC1.
- Firmware display version, USB product string and web editor identify the release as RC1.

## Retained from TEST21

- 128-slot USER bank bootstrap (38 recovered community patches plus recovered factory copies), preserving already-valid USER slots.
- Settings-only generative mutation controls.
- 303 piano roll / Gate / Octave / Accent / Slide editor modes.
- Note-edge declicking and live running-sequence octave transpose.
- USB-C Web MIDI editor.
- FWSC-only normal update path; generic M-Flasher remains disabled to protect per-device USER-bank data.
