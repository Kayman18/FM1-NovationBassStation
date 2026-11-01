# FM1 Bass Station TEST18 source

TEST18 is the 240×240 Bass Station GUI + splash-screen source update.

## What changed

- true page-based 240×240 FM1 UI rather than a desktop-style all-controls screen
- Bass Station black/charcoal + cyan/white visual theme
- four large K1–K4 parameter cards on normal pages
- HOME / PERFORMANCE screen
- redesigned preset browser
- redesigned integrated 303 sequencer pages
- approved Novation / Bass Station splash screen embedded as a 240×240 16-colour 4-bpp asset
- TEST17 synth/preset/user-bank behavior preserved
- top-level `build.sh` updated for TEST18 / 0.3.0

## Validation completed

- host build: PASS
- existing TEST17 UI/MIDI smoke regression: PASS
- target firmware build: PASS
- sequencer + Bass Station unit tests: PASS
- generated TEST18 FWSC and M-Flasher image validated
- hardware flash test: not performed

## Reconstruct the complete project source

The GitHub connector cannot directly commit the binary source archive, so this folder stores two verified base64/XZ overlays. They reconstruct the exact 77-file TEST18 project source from the already-published TEST15 source archive.

From the repository root:

```bash
bash TEST18/source-bootstrap/reconstruct-test18.sh
```

This creates:

- `FM1-BassStation-TEST18-Project-Source/`
- `FM1-BassStation-TEST18-Project-Source.tar.xz`

The reconstructed source tree is checked against:

```
f18068d5ed54aa0e201305a56c401f8630f722e752042828e88fe8e0a3cb45e5
```

The reconstructed source contains the complete TEST18 app/engine changes, splash preview + packed firmware asset, `build.sh`, tests, research notes and build-support files.
