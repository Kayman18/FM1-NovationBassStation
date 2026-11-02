# FM1 Bass Station TEST21 — generative mutation settings + populated USER bank

TEST21 keeps the TEST20 sequencer declick and live running-sequence OCT-/OCT+
transpose fixes, while restoring the MUTATE page to the intended generative
workflow and preloading the USER bank safely.

## MUTATE page restored to settings-only behavior

The four MUTATE-page knobs no longer alter the current pattern while they are
turned:

- K1 — TIME OPS, 1..8
- K2 — NOTE/OCT OPS, 1..8
- K3 — ACC/SLD OPS, 1..8
- K4 — BLOCK SEED, unlocked/locked

The settings describe what the **next** mutation will do. ENV applies TIME,
LFO applies NOTE/OCT, GLO applies ACC/SLIDE, and FX performs the one-level undo.
REC/GENERATE creates a new seed while BLOCK is off; with BLOCK on it preserves
the seed and applies all three configured mutation families once.

No mutation happens merely because a step/note is playing.

## USER bank preload

TEST21 fills empty USER slots once, without overwriting any valid saved USER
patches. A persistent marker prevents deliberately cleared slots from being
repopulated on every boot.

- USER 1..5: Old School Jungle / Chopper patches.
- USER 6..38: Pro-Sounds BassStation Leads patches.
- USER 39..128: recovered factory-program copies 00..89, used to make the full
  128-slot USER bank immediately useful.

The 38 community patches are the exact native 34-byte Bass Station plug-in
records previously recovered for the TEST16 COMMUNITY-bank experiment. See
`community/README.md` for attribution and source archive hashes.

## Preserved TEST20 fixes

- short final-amplitude note-edge slew to reduce clicks at zero/near-zero
  attack/release settings;
- OCT-/OCT+ changes `seq.base_octave` live while the 303 transport is running;
- normal FWSC updates preserve the per-device USER-bank flash sectors;
- generic full-image M-Flasher remains opt-in clean-install/recovery only.

## Testing target

Hardware verification should confirm:

1. turning K1-K4 on MUTATE changes only the displayed settings;
2. ENV/LFO/GLO then apply the configured mutation amount once;
3. all 128 USER slots are populated after the first TEST21 boot;
4. existing valid USER patches survive the preload pass unchanged;
5. sequencer clicks remain reduced and OCT-/OCT+ transpose live playback.

## Validation

- host simulator build: PASS
- full UI/preset/sequencer persistence script: PASS
- blank USER-bank bootstrap: 128/128 slots populated
- existing valid USER slots preserved during bootstrap: PASS
- mutation knobs settings-only regression: PASS
- sequencer / engine / factory / Rack SysEx / bank tooling unit tests: PASS
- target application size: 156,992 bytes
- target application SHA-256: `82b836d9eabf6b647fbeee07492014bfc2d9f11a97c4f3d3940cb2acb1f6705a`
- FWSC size: 610,108 bytes
- FWSC SHA-256: `6ff09ea142a48f9a3c819a5316befa3d4131a3a69efaabb59253fa34b8678e69`
- clean source-archive rebuild reproduces both the application image and FWSC exactly
- package identity: `FM-1_20000401` / display version `TEST21`

Hardware audio/display/USB-C behavior still requires real FM1 testing.
