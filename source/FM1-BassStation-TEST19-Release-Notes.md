# FM1 Bass Station TEST19 — 303 multi-mode editor + USB-C web editor

## Summary

TEST19 is the sequencer/editor release built on TEST18. It preserves the Bass
Station DSP, recovered factory programs, 128-slot USER bank, preset browser,
240×240 Bass Station GUI and approved Novation/Bass Station splash.

The integrated 303-style sequencer now exposes the editor functions that were
previously hidden behind button shortcuts, and the same synth/sequencer state is
controllable from a local browser over the FM1 USB-C MIDI connection.

## FM1 sequencer views

The SEQ page now cycles through seven dedicated 240×240 views:

- GENERATE
- MUTATE
- PIANO ROLL
- GATE (Rest / Note / Tie)
- OCTAVE (-2..+2; combines the reference editor's UP and DOWN rows)
- ACCENT
- SLIDE

The editor is intentionally paged rather than attempting to show every lane on
the 240×240 display. Four bars of 16 steps provide access to all 64 stored
steps. Piano-roll rows outside the current scale are visibly recessed/inactive.

## Mutation controls

The three mutation families now have visible, adjustable operation counts:

- TIME: default 2 operations
- NOTE/OCT: default 2 operations
- ACC/SLIDE: default 3 operations

Each can be adjusted from 1 to 8 operations. The values persist in the existing
sequencer settings bytes that were reserved in TEST17/18, preserving settings
structure size and compatibility. BLOCK still locks the pattern seed; REC while
BLOCK is active applies all three mutation families. One-level undo remains
available.

## USB-C browser control

`web-editor/` adds a self-contained editor patterned after the FM1 V-Station
USB-C editor. It uses Web MIDI SysEx over the existing class-compliant USB MIDI
port; no Ethernet/Wi-Fi stack or web server is embedded in the firmware.

The browser protocol can:

- handshake and identify TEST19
- read/write all 48 exposed synth parameters
- load factory/USER programs and save USER programs
- request complete synth state
- request complete 64-step sequencer state
- edit sequencer generation/mutation controls
- edit individual step pitch, octave, gate state, accent and slide
- generate, mutate, undo, clear and start/stop transport
- save the current synth + sequencer settings persistently to FM1
- panic/all-notes-off

The webpage includes ALL, PIANO, GATE, OCTAVE, ACCENT and SLIDE editor views,
four 16-step pages, mutation controls, preset controls and JSON import/export.

## ATEK303SEQ behavior reference

The editor model was checked against Animatek's public ATEK303 SEQ manual and
source behavior: 64 steps, one-octave piano roll, Rest/Note/Tie time states,
UP/DOWN octave displacement, accent, outgoing slide, deterministic seed lock,
three mutation families and one-level undo. The FM1 implementation remains a
clean-room implementation and does not copy GPL implementation code.

## Validation

TEST19 is checked with:

- dedicated `acid_seq_test` unit coverage
- the full Bass Station host UI/audio/preset regression script
- browser SysEx host commands for synth, sequencer controls and individual steps
- JavaScript syntax validation for `web-editor/index.html`
- target firmware build and M-Flasher validation
- 240×240 host-rendered screen captures for all seven sequencer views

Hardware flashing is still a separate real-device test; successful host and
target builds do not claim physical USB/browser or display validation until the
firmware is tested on the FM1.
