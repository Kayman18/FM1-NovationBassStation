# FM1 Bass Station TEST18 — 240×240 Bass Station GUI + splash

## Status

TEST18 keeps the TEST17 Bass Station engine, recovered factory bank, USER bank,
preset browser and integrated 303 sequencer, and adds the first complete
Bass Station-themed FM1 display pass.

The target firmware and host simulator both build successfully. The existing
TEST17 host regression script passes unchanged after the GUI work.

## New 240×240 GUI

The previous dense text view has been replaced with a page-based layout aimed
at the physical FM1 screen rather than a desktop plug-in panel.

- black / charcoal Bass Station theme
- cyan-blue Novation/Bass Station accents
- larger white parameter values
- four large K1–K4 parameter cards per normal page
- one page at a time; no attempt to fit the whole synth on screen
- persistent preset/status footer with direct control hints
- dedicated HOME / PERFORMANCE overlay
- redesigned preset browser
- redesigned 303 sequencer control pages and step editor
- cyan FM1 base palette selected for the rest of the platform UI

The existing page/control mappings remain intact so the GUI change does not
re-map the synth engine.

## HOME / PERFORMANCE

Pressing `HOME` opens a compact performance screen with four immediately useful
controls:

- K1: filter cutoff
- K2: resonance
- K3: oscillator mix
- K4: volume

The header also shows a small OSC1 / OSC2 / filter-slope summary. Turning the
SELECT encoder leaves HOME and returns to the normal page browser. This HOME
view is transient; saved TEST17 page numbers and preset data stay compatible.

## Splash screen

TEST18 enables the FM1 custom splash hook and embeds the approved Novation Bass
Station startup artwork at 240×240. The accepted composition is scaled to the
square FM1 screen with Novation above the synth and Bass Station below it.

To keep flash use modest, the splash is stored as a 16-colour 4-bpp indexed
image:

- 240×240 display image
- 28,800 bytes of packed pixel indices
- 16-entry RGB565 palette
- decoded through the existing FM1 display canvas at boot

A PNG preview is included at `app/assets/bass_station_splash_240.png`; firmware
uses the generated packed data in `app/splash_bassstation.h`.

## Validation

Completed in this pass:

- `bash build.sh --host`: PASS
- `app/build/host/bassstation_host tests/ui_smoke.script ...`: PASS
- `bash build.sh`: PASS
- target application size: 145,452 bytes
- target application slot: 581,564 bytes
- RAM `.data + .bss`: 23,040 / 98,304 bytes
- pool: 69,124 / 344,064 bytes
- TEST18 FWSC generated successfully
- TEST18 M-Flasher image generated and validated

No hardware flash/test was performed in this pass.

## Build outputs

`build.sh` now identifies TEST18 / release 0.3.0 and creates:

- `release/FM1-BassStation-TEST18.fwsc`
- `release/FM1-BassStation-TEST18-M-Flasher.bin`
- `release/FM1-BassStation-TEST18-M-Flasher.json`

The full source package continues to include the top-level `build.sh`, base
framework, build dependencies, compressed JieLi toolchain, pinned M-Flasher
reference image, Bass Station engine, bank tools, source tests and sequencer.
