# TEST17 303-style sequencer integration

Reference behavior: Animatek ATEK303 SEQ, VCV Library and public user manual.

- https://library.vcvrack.com/Animatek/ATEK303SEQ
- https://github.com/animatek/UZZ-VCV-RACK/blob/main/Manuals/ATEK303SEQ_Manual_EN.md

ATEK303SEQ is GPL-3.0-or-later. TEST17 does **not** copy its implementation.
The FM1 sequencer in `app/acid_seq.c` is a clean-room MPL-2.0 implementation
written from the public behavior described by the manual: up to 64 steps,
Rest/Note/Tie timing, deterministic seeded generation, root/scale playback,
accent/slide articulation, three mutation families and one-level undo.

## FM1 control mapping

- `SEQ`: open the 303 sequencer. Repeated presses cycle its three control pages;
  in step edit it advances to the next 16-step page.
- `PLAY`: start/stop transport.
- `REC`: GENERATE. With BLOCK off a new seed is chosen; with BLOCK on all
  mutation families are applied while the seed stays fixed.
- `ARP`: BLOCK / lock seed.
- `ENV`: mutate time layer.
- `LFO`: mutate note/octave layer.
- `GLO`: mutate accent/slide layer.
- `FX`: one-level undo.
- `EDIT`: enter/leave the step editor.
- `SEL`: preset browser remains available from the sequencer.
- `SAVE`: saves the global sequence with settings.

### Control page 1

K1 BPM, K2 STEPS (1..64), K3 NOTES density, K4 RANGE.

### Control page 2

K1 ROOT, K2 SCALE, K3 GATE, K4 ACCENT density.

### Control page 3

K1 SLIDE density, K2 BASE OCTAVE, K3 tempo-relative GLIDE time, K4 CLOCK
(INTERNAL or MIDI).

### Step editor

The display shows one 16-step page at a time. `SELECT` moves one step,
`PRESET` moves one 16-step page. K1 edits scale degree, K2 octave, K3 cycles
Rest/Note/Tie and K4 cycles plain/accent/slide/accent+slide.

## Playback integration

The internal clock is sample-counted in `app_render()` rather than driven by
screen/UI timing. MIDI-clock mode advances one sequencer step per six MIDI
clock pulses (a sixteenth note). Accent becomes note velocity. Ties leave the
Bass Station gate/envelopes alive. Slides use the new `bassstation_note_slide()`
entry point, which performs a tempo-relative legato pitch ramp without
changing the loaded patch's PORTA or trigger-mode parameters.

## Persistence scope

TEST17 deliberately stores one global 64-step sequence in the settings object
rather than enlarging each USER patch. This preserves the existing 128-slot
USER bank format and its TEST15/TEST16 compatibility. A later USB-C HTML editor
can add a dedicated pattern library/export format without consuming the patch
bank's flash budget.
