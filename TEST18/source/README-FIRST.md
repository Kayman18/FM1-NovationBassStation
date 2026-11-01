# FM1 Bass Station TEST18 — GUI / splash build

TEST18 is the Bass Station FM1 port with the new 240×240 multipage GUI and the
approved Bass Station startup splash integrated into the build.

The sound engine, 100 recovered factory programs, 128 USER slots, preset
browser and TEST17 303-style sequencer remain in place.

## Build

    bash build.sh

The build writes:

    release/FM1-BassStation-TEST18.fwsc
    release/FM1-BassStation-TEST18-M-Flasher.bin
    release/FM1-BassStation-TEST18-M-Flasher.json

## Host simulator

    bash build.sh --host
    app/build/host/bassstation_host tests/ui_smoke.script tests/ui_out

## GUI navigation

The FM1 screen is deliberately multipage. Normal synth pages show at most four
large parameters mapped directly to K1–K4.

- HOME: HOME / PERFORMANCE overlay (cutoff, resonance, oscillator mix, volume)
- EDIT: OSC2 / OSC modulation pages
- FX: FILTER
- ENV: AMP ENV / MOD ENV / TRIGGER + VELOCITY
- LFO: LFO
- GLO: GLOBAL / AFTERTOUCH / BREATH
- SEQ: integrated 303 sequencer
- SEL: preset browser
- SELECT encoder: cycle normal synth pages
- PRESET encoder: change patches

The HOME screen is a transient overlay; the existing TEST17 page numbering and
saved synth data remain compatible.

## Splash asset

The firmware splash is generated from the approved Novation Bass Station
composition and stored as a compact 16-colour, 4-bpp 240×240 image.

- PNG preview: `app/assets/bass_station_splash_240.png`
- packed firmware data: `app/splash_bassstation.h`

See `FM1-BassStation-TEST18-Release-Notes.md` for validation and implementation
details.
