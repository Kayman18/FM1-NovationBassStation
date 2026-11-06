# TEST19 ATEK303SEQ behavior integration

Reference behavior was checked against Animatek's public ATEK303 SEQ VCV Rack
module and manual:

- https://library.vcvrack.com/Animatek/ATEK303SEQ
- https://github.com/animatek/UZZ-VCV-RACK/blob/main/Manuals/ATEK303SEQ_Manual_EN.md

ATEK303SEQ is GPL-3.0-or-later. The FM1 code in `app/acid_seq.c` / `.h` is a
clean-room MPL-2.0 implementation based on documented behavior; no GPL source is
copied into the FM1 firmware.

## Behavior represented on FM1

The reference editor stores up to 64 steps in four pages of 16 and exposes a
one-octave piano roll plus UP, DOWN, GATE, ACC and SLIDE rows. TEST19 maps those
ideas to separate readable 240×240 modes:

- `GENERATE`: musical generation controls
- `MUTATE`: mutation strengths + BLOCK seed lock
- `PIANO ROLL`: one chromatic octave; rows outside the current scale are dimmed
- `GATE`: Rest / Note / Tie
- `OCTAVE`: -2 / -1 / 0 / +1 / +2, combining UP and DOWN into one lane page
- `ACCENT`: per-note accent
- `SLIDE`: outgoing slide

This is intentionally not a screenshot-sized copy of the VCV panel. Splitting
lanes across pages gives each control enough space to remain legible on FM1.

## FM1 controls

- `SEQ`: enter sequencer / cycle through the seven modes
- `EDIT`: quick GENERATE <-> PIANO ROLL toggle
- `SELECT`: choose step while editing
- `PRESET`: choose 16-step bar while editing
- `PLAY`: start/stop transport
- `REC`: generate a new identity; with BLOCK on, mutate all families
- `ARP`: BLOCK / lock seed
- `ENV`: mutate time
- `LFO`: mutate note/octave
- `GLO`: mutate accent/slide
- `FX`: one-level undo
- `SAVE`: persist global sequencer state with FM1 settings

### Generation controls

The three GENERATE panels retain BPM, loop length (1..64), note density, range,
root, scale, ordinary gate length, accent density, slide density, base octave,
slide time and internal/MIDI clock selection.

### Mutation controls

TEST19 makes the mutation operation counts persistent and adjustable:

- time: 1..8, default 2
- note/octave: 1..8, default 2
- accent/slide: 1..8, default 3

The defaults match the reference module's documented button actions. BLOCK
keeps the current seed identity and turns GENERATE into a combined mutation.

## Pattern data and editing

Each step stores:

- time kind: Rest, Note or Tie
- scale degree for Note steps
- octave displacement -2..+2
- accent flag
- outgoing slide flag

The pattern remains 64 steps in storage even when the active loop is shorter.
Pitch edits use the active root/scale mapping. Invalid slide combinations are
normalized away by the sequencer engine.

## Playback integration

The FM1 version intentionally adapts clocking to the hardware instead of
requiring a VCV clock cable. INTERNAL clock is sample-counted in `app_render()`.
MIDI clock advances one sequencer sixteenth every six MIDI clocks. Accent maps
to note velocity; Tie holds the existing gate/envelopes; Slide uses the Bass
Station port's tempo-relative legato pitch ramp without rewriting patch PORTA
or trigger-mode settings.

## USB-C web editor

TEST19 adds a browser editor using the same design as the V-Station FM1 tool:
class-compliant USB MIDI + Web MIDI SysEx. The firmware exposes one shared live
state instead of maintaining a separate web copy.

The protocol can exchange all exposed synth parameters and all 64 sequencer
steps, including generation values, mutation strengths, seed lock, transport,
Rest/Note/Tie, pitch, octave, accent and slide. Browser edits therefore use the
same state and normalization functions as hardware edits.

The browser provides ALL / PIANO / GATE / OCTAVE / ACCENT / SLIDE subviews and
four 16-step pages. A local helper HTTP server is supplied only because browsers
require a secure/localhost origin for Web MIDI; the FM1 itself does not host an
HTTP server.

## Deliberate differences from the VCV module

- The FM1 can use its own internal clock or USB/DIN MIDI clock rather than an
  external modular clock input.
- UP/DOWN are combined into one five-lane OCTAVE hardware page; the web editor
  still presents separate UP and DOWN rows.
- FM1 slide drives the integrated Bass Station voice directly.
- Context-menu features that depend on VCV files/expander/CV routing (for
  example VCV expander routing) are not copied as hardware modes.
