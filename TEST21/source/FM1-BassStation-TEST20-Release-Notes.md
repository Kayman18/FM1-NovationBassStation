# FM1 Bass Station TEST20 — direct mutation-knob fix

TEST20 fixes the TEST19 MUTATE-page control mismatch reported on hardware.

## Fix

The four MUTATE-page encoders now match the approved FM1 layout:

- **K1 TIME MUTATE** — turning the knob immediately applies a time-layer mutation.
- **K2 NOTE/OCT MUTATE** — turning the knob immediately applies a pitch/octave mutation.
- **K3 ACC/SLIDE MUTATE** — turning the knob immediately applies an articulation mutation.
- **K4 AMOUNT** — sets shared mutation depth from 1 to 8. TIME and NOTE use N operations; ACC/SLIDE uses N+1 (clamped to 8), so the default remains 2/2/3.

The ENV/LFO/GLO mutation buttons remain as shortcuts, ARP still controls BLOCK, and FX still provides one-level undo.

All TEST19 piano-roll, gate, octave, accent, slide, 64-step/bar navigation, USB-C Web MIDI editor and Bass Station synth features are retained.


## Additional hardware fixes

### Sequencer note-edge declick

TEST20 adds a very short final-amplitude slew limiter. It only affects amplitude
edges faster than about 1.45 ms, so ordinary patch attack/release times are
unchanged. This targets the clicks heard when the sequencer repeatedly triggers
patches with zero/near-zero amp attack or release. Ties and slides remain legato.

### Live OCT-/OCT+ while the sequencer runs

While the 303 transport is running, the physical octave buttons now change the
sequencer's `base_octave` (C1..C5) without stopping or regenerating the pattern.
When the sequencer is stopped, the buttons retain their existing touch-keyboard
octave behavior.

## USER-bank preservation / M-Flasher warning

The USER-bank storage map and bank record format are unchanged from TEST9 through
TEST20. The normal FWSC update loader writes only the application area
`0x4000..0x92fff` and explicitly leaves the app store/user-data area untouched.

The generic 1 MiB M-Flasher image is different: it is constructed from a clean,
SHA256-pinned TEST81 reference backup and therefore contains that reference
image's data outside the application region, not the unique USER bank currently
on a particular FM1. A full-image M-Flasher flash can therefore replace a
device's saved USER bank. TEST20 should be installed with the FWSC when preserving
existing USER patches matters. The generic M-Flasher is a clean-install/recovery
image, not a user-data-preserving updater.
