# FM1 Bass Station TEST19 — full 303 editor + USB-C web control

TEST19 keeps the TEST18 Bass Station synth engine, factory/USER banks, 240×240
Bass Station GUI and approved splash, then expands the integrated 303-style
sequencer into a full multi-view editor and adds direct USB-C browser control.

## Build

Run:

```sh
bash build.sh
```

The target build produces:

- `release/FM1-BassStation-TEST19.fwsc`
- `release/FM1-BassStation-TEST19-M-Flasher.bin`
- `release/FM1-BassStation-TEST19-M-Flasher.json`

`build.sh --host` builds the desktop host simulator used by the regression suite.

The compact GitHub source does not duplicate the 25 MB third-party Jieli compiler archive. For a target build, set `JIELI_TOOLCHAIN` to an extracted `jieli-linux-toolchains-20260730.1` directory or place that archive under `toolchain/`.

## 303 sequencer on the 240×240 FM1 display

Press `SEQ` to enter the sequencer. Repeated `SEQ` presses cycle seven readable
views instead of compressing the whole editor onto one screen:

1. **GENERATE** — BPM, length, note density, range, root, scale, gate, accent,
   slide, base octave, slide time and clock source across three control panels.
2. **MUTATE** — visible TIME, NOTE/OCT and ACC/SLIDE mutation strengths plus
   BLOCK/seed lock. Mutation strengths are persistent and adjustable 1..8.
3. **PIANO ROLL** — one chromatic octave, with scale-disallowed rows dimmed.
4. **GATE** — Rest / Note / Tie lane editing.
5. **OCTAVE** — per-step -2 / -1 / 0 / +1 / +2 editing (the ATEK UP/DOWN
   behavior combined into one clearer FM1 page).
6. **ACCENT** — per-step accent editing.
7. **SLIDE** — per-step outgoing slide editing.

Patterns remain 64 steps long internally, displayed as four bars of 16.
`SELECT` chooses a step and `PRESET` changes the 16-step bar in edit views.

Important buttons on the SEQ page:

- `PLAY`: start/stop
- `REC`: GENERATE; with BLOCK enabled it mutates all three families
- `ARP`: toggle BLOCK/seed lock
- `ENV`: mutate time
- `LFO`: mutate note/octave
- `GLO`: mutate accent/slide
- `FX`: undo the last mutation/edit snapshot where available
- `EDIT`: toggle quickly between GENERATE and PIANO ROLL

## USB-C web editor

`web-editor/` is a self-contained local editor modeled on the V-Station workflow.
The webpage communicates directly with the FM1 over class-compliant USB MIDI
SysEx; it does **not** require networking firmware on the FM1.

Start the included local server (`start-editor.sh` on Linux/macOS or
`start-editor.bat` on Windows), open the localhost page in Chrome/Edge, then
press **CONNECT USB-C** and choose the FM1 Bass Station MIDI port.

The browser editor provides:

- all 48 exposed Bass Station synth controls
- factory and USER preset loading and USER saving
- JSON save/load on the computer
- full 64-step sequencer editing
- generation parameters and seed lock
- variable mutation strengths, generate/mutate/undo/clear/transport actions
- piano, gate, octave, accent and slide views matching the FM1 editor
- **SAVE TO FM1** for persistent synth + sequencer settings

The browser and hardware use the same live state. Hardware knobs/buttons,
MIDI CC/program changes and webpage edits are reflected through the same
parameter/sequencer layer.

## Compatibility

TEST19 deliberately retains the TEST17/TEST18 settings magic and USER-bank
layout. Existing saved patches and the global sequence format remain compatible;
the three bytes previously reserved in the sequencer state now hold mutation
strengths and default to ATEK-style 2 / 2 / 3 operations.

See `FM1-BassStation-TEST19-Release-Notes.md` and
`research/ATEK303SEQ-INTEGRATION.md` for implementation and validation details.
