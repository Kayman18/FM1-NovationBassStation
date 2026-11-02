# FM1 Bass Station TEST16 — source-only clock / packaging fidelity pass

## Status

TEST16 is a **source-only** continuation of TEST15. No TEST16 firmware image was built or flashed in this pass.

## Native LFO-sync internal-index correction

The earlier v2.2 compact-state audit recognized native sync raw values `0,2,3,4,6,8,9,12,16,18,24,32,36,48,72,96` and treated compact raw 64 as OFF. In that compact list, raw 72/96 were decoded to software public indices 14/15.

FM1's runtime clock list also preserves the Rack/public 64-clock `1 Bar Triplet` division at internal index 14. TEST15 copied the compact software index directly, so an imported native raw 72/96 state would run as 64/72 clocks. TEST16 translates already-recognized native raw 72/96 to internal indices 15/16, producing the intended 72/96-clock periods while leaving Rack import/export unchanged.

The software plug-in manual itself lists a public 64-clock `1 Bar Triplet` entry. That conflicts with the earlier static compact-loader result that treated raw 64 as invalid/off. TEST16 records this as an open serialization question and does **not** guess a new compact raw-64 mapping until the original DLL lookup is re-audited.

## Pulse-width audit — no DSP change

A fresh check against the **software plug-in** manual (rather than the original hardware manual) confirms TEST15's pulse-width interpretation:

- MAN: centre value 0 is the 50% square wave; moving either direction narrows the pulse toward opposite polarities.
- PWM LFO: centre is no effect; either direction increases modulation depth with opposite polarity.
- ENV2: uses the same centre-off depth concept.
- the PWM LFO is an additional fixed-speed triangle LFO, separate from the main variable LFO.

Therefore the proposed one-sided MAN mapping was rejected and no PW DSP change is present in TEST16.

## Buildable source package restored

The lean TEST15 project archive contained `build.sh` but omitted directories it references. TEST16 restores the unchanged generic build support from the last complete source kit:

- `base/`
- `toolchain/jieli-linux-toolchains-20260730.1.tar.xz`
- `reference/FM1-PolyWavetable-TEST81-M-Flasher.bin`
- complete `build-deps/` support

`build.sh` is present at the package root and identifies TEST16 / release 0.2.8. It was syntax-checked only; it was **not run** during this source-only pass.

## Source regression changes

- native sync raw 48 -> internal 13 (48 clocks)
- native sync raw 64 -> unchanged OFF pending DLL re-audit
- native sync raw 72 -> internal 15 (72 clocks)
- native sync raw 96 -> internal 16 (96 clocks)
- Rack raw 64 remains internal 14 and round-trippable

## Remaining fidelity targets

- re-audit the v2.2 compact sync lookup specifically for raw 64
- exact fixed PWM-LFO rate
- oscillator mixer/gain law
- cutoff/resonance response
- normalized-to-physical envelope/LFO/portamento curves
- pitch/filter modulation depth tapers
