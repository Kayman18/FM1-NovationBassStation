# FM1 Bass Station TEST19

TEST19 adds the full multi-view 303-style sequencer editor and USB-C Web MIDI browser editor on top of TEST18.

## FM1 sequencer views

- Generate
- Mutate
- Piano Roll
- Gate (Rest / Note / Tie)
- Octave (-2 through +2)
- Accent
- Slide

Patterns store 64 steps in four 16-step bars. Mutation strengths are adjustable and persistent (defaults 2 / 2 / 3).

## USB-C web editor

Download `FM1-BassStation-TEST19-Web-Editor.zip`, run the included local server, open it in Chrome/Edge and connect to the FM1 MIDI port. The browser can edit the synth, presets, complete sequencer, mutation controls and piano-roll/articulation lanes over Web MIDI SysEx.

## Files

- `FM1-BassStation-TEST19.fwsc` — firmware update package
- `FM1-BassStation-TEST19-M-Flasher.bin` — M-Flasher image
- `FM1-BassStation-TEST19-M-Flasher.json` — validation metadata
- `FM1-BassStation-TEST19-Web-Editor.zip` — local USB-C editor
- `FM1-BassStation-TEST19-Project-Source.tar.xz` — verified source archive
- `source/` — browsable source tree, populated automatically after archive verification

Target application: 154612 bytes. Host regression, sequencer unit tests and source-rebuild verification pass. Hardware flash / physical USB-C testing is not claimed yet.
