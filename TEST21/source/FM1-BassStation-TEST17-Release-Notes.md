# FM1 Bass Station TEST17 — source-only 303 sequencer integration

## Status

TEST17 is a **source-only** continuation of TEST16. `build.sh` is included and
updated for TEST17, but it was not executed and no TEST17 firmware image was
built or flashed in this pass.

## Integrated 303-style sequencer

The FM1 `SEQ` button now opens a dedicated monophonic acid sequencer inspired
by the public behavior of Animatek ATEK303 SEQ. The implementation is new
MPL-2.0 code; no GPL ATEK303SEQ source is included.

Core behavior:

- up to 64 steps, shown/edited in four 16-step pages
- Rest / Note / Tie timing states
- scale-degree pitch plus per-step octave displacement
- per-step Accent and outgoing Slide
- deterministic seeded pattern generation
- BLOCK / seed lock
- time, note/octave, and accent/slide mutation families
- one-level mutation undo
- 8 scales: Acid, Natural Minor, Phrygian, Harmonic Minor, Dorian, Blues,
  Major, and Chromatic
- root transposition, note density, melodic range, gate length, accent density,
  slide density, base octave, and slide-time controls
- internal 30..300 BPM sixteenth-note clock or MIDI-clock mode

## FM1 controls

While on the sequencer page:

- `SEQ`: cycle the three control panels. In step edit, move to the next
  16-step page.
- `PLAY`: start / stop transport.
- `REC`: GENERATE. BLOCK off selects a new seed; BLOCK on performs a full
  mutation while retaining the seed.
- `ARP`: BLOCK / lock seed.
- `ENV`: mutate the Rest/Note/Tie time layer.
- `LFO`: mutate note/octave material.
- `GLO`: mutate accents/slides.
- `FX`: undo the last mutation.
- `EDIT`: enter / leave step edit.
- `SEL`: still opens the TEST15 preset browser.
- `SAVE`: stores settings and the current sequence.

Control panel 1: BPM, STEPS, NOTES, RANGE.

Control panel 2: ROOT, SCALE, GATE, ACCENT.

Control panel 3: SLIDE, BASE OCT, GLIDE, CLOCK.

In step edit, SELECT moves one step, PRESET moves by 16 steps, and K1..K4 edit
pitch, octave, Rest/Note/Tie state, and articulation respectively.

## Bass Station engine integration

A new `bassstation_note_slide()` entry point performs a legato pitch ramp while
leaving the amp/mod envelopes running. The sequencer uses this for 303-style
slides with a tempo-relative slide time. It does **not** alter the loaded
Bass Station patch's trigger mode or PORTA value.

Accent maps to note velocity. Tie holds the existing note and gate without a
new envelope attack. Normal notes use the sequencer gate percentage unless the
next step is a Tie or a valid Slide target.

Internal sequencer timing is sample-counted inside `app_render()`, allowing
note and gate boundaries to split a render block instead of depending on UI
frame timing. MIDI-clock mode advances once every six MIDI clocks (one
sixteenth note) and responds to MIDI Start/Continue/Stop.

Active-pattern wrap is handled explicitly: a Slide on the final active step
(15/31/47/63 as applicable) targets step 0, not a hidden step outside the
current pattern length.

## Persistence / compatibility

TEST17 bumps the settings record from BST7 to BST8 and migrates TEST15/TEST16
settings automatically. Existing Bass Station synth parameters, built-in
factory programs, and the 128-slot USER bank format are unchanged.

The 64-step sequencer is currently **global**, stored in the settings object,
rather than duplicated inside each USER patch. This preserves all 128 USER
slots and avoids changing the bank flash layout. A later USB-C HTML editor can
add a pattern library/import/export layer separately.

## Buildable source package

As in TEST16, the source package contains:

- top-level `build.sh`
- `base/`
- `build-deps/`
- compressed JieLi toolchain
- pinned M-Flasher reference image
- Bass Station engine/app source
- sequencer source in `app/acid_seq.c` / `.h`
- source tests and research notes

`build.sh` identifies TEST17 / release 0.2.9 and would create TEST17 FWSC and
M-Flasher outputs if the user chooses to run it locally.

## Not built in this pass

No target build, host build, FWSC, M-Flasher image, flash write, or hardware
validation was performed for TEST17. The next hardware build should explicitly
check sequencer CPU cost, gate timing, slide behavior, MIDI-clock stability,
settings migration and screen readability.
