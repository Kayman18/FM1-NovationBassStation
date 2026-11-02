# FM1 Bass Station RC1

RC1 is the current FM1 Bass Station test source.

It retains the TEST20 note-edge declick, live sequencer octave transpose,
240x240 multipage GUI, full seven-view 303 editor, preset browser, recovered
100-program factory bank, 128-slot persistent USER bank, and USB-C Web MIDI
editor.

## RC1 changes

### Generative mutation workflow

The MUTATE page is settings-only:

- K1 = TIME OPS
- K2 = NOTE/OCT OPS
- K3 = ACC/SLD OPS
- K4 = BLOCK SEED

Turning these knobs does not modify the pattern. ENV/LFO/GLO apply the three
mutation families. REC/GENERATE makes a new pattern with BLOCK off and applies
all three configured mutations with BLOCK on.

### Populated USER bank

On the first RC1 boot, empty USER slots are filled once. Existing valid USER
patches are never overwritten.

- slots 1..38: recovered Rekkerd community patches (5 Chopper Jungle + 33
  Pro-Sounds leads)
- slots 39..128: recovered factory programs 00..89 copied into USER storage

The preload is stored in the normal USER-bank flash objects, so patches can be
edited and overwritten normally afterward. A seed marker prevents intentional
clears from being refilled automatically on later boots.

## Safe update path

Use `release/FM1-BassStation-RC1.fwsc` for normal updates. It updates the
application region and preserves the per-device USER-bank storage sectors.

A generic full 1 MiB M-Flasher is not built unless
`BUILD_CLEAN_MFLASHER=1` is explicitly set. Such an image is for clean-install
or recovery and cannot preserve the USER bank already stored on a particular
FM1.

## Building

The package includes `build.sh`, the FM1 base source and build support. Supply
an extracted Jieli toolchain with `JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1`
or place the matching archive in `toolchain/`.

    bash build.sh

Host simulator:

    bash build.sh --host

## Community preset provenance

See `community/README.md` and `THIRD-PARTY-NOTICES.md`.
