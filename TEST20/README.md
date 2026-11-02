# FM1 Bass Station TEST20

TEST20 fixes the hardware issues found in TEST19 and keeps the full 303 editor / USB-C web editor.

## Fixed

- MUTATE page: K1 TIME, K2 NOTE/OCT and K3 ACC/SLIDE now mutate immediately.
- K4 sets shared mutation amount while retaining the original 2 / 2 / 3 default behavior.
- A short final-amplitude slew suppresses clicks from zero/near-zero attack/release note edges.
- While the 303 sequencer is running, OCT-/OCT+ transpose its base octave live without stopping or regenerating the pattern.
- Existing USER-bank storage layout is unchanged.

## Important USER-bank safety

Use **FM1-BassStation-TEST20.fwsc** for the normal update. The FWSC loader writes only the application area and leaves the USER-bank store untouched.

A generic 1 MiB M-Flasher is intentionally not published in TEST20. Such an image is built from a clean reference backup and cannot preserve the unique USER-bank sectors currently stored on an individual FM1.

If TEST19 was installed with the generic M-Flasher and the USER bank became empty immediately afterward, recovery requires an earlier full-flash backup or an exported Bass Station USER-bank/SysEx copy.

## Files

- FM1-BassStation-TEST20.fwsc — recommended firmware update
- FM1-BassStation-TEST20-Web-Editor.zip — USB-C Web MIDI editor
- FM1-BassStation-TEST20-Project-Source.tar.xz — exact rebuildable source archive
- source/ — browsable source tree, populated by GitHub Actions after archive verification

Target application size: 154,996 bytes. Host UI, synth engine, sequencer and exact source-rebuild checks pass. Hardware testing is still required for the audible declick result.
