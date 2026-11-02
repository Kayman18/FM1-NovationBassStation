# FM1 Bass Station TEST21

TEST21 restores the 303 MUTATE page to its intended generative workflow and safely fills the 128-slot USER bank.

## Mutation behavior

The MUTATE knobs are settings only:

- K1 = TIME OPS (1..8)
- K2 = NOTE/OCT OPS (1..8)
- K3 = ACC/SLD OPS (1..8)
- K4 = BLOCK SEED

Turning K1-K4 does not change the current sequence. ENV, LFO and GLO apply the corresponding mutation once. Generate creates a new seed with BLOCK off; with BLOCK on it retains the seed and applies all configured mutation families once.

## USER bank bootstrap

On first TEST21 boot, only empty USER slots are filled:

- USER 1-5: Old School Jungle / Chopper patches
- USER 6-38: Pro-Sounds BassStation Leads
- USER 39-128: recovered Bass Station factory programs 00-89

Existing valid USER patches are never overwritten. A persistent seed marker prevents deliberately cleared slots from being repopulated on later boots.

## Other retained fixes

TEST21 keeps the TEST20 note-edge declick and live OCT-/OCT+ sequencer transpose behavior.

Use the **FWSC** for normal updates. No generic M-Flasher is included because a generic full-flash image cannot preserve per-device USER-bank data.

## Validation

- target application: 156,992 bytes
- app SHA-256: `82b836d9eabf6b647fbeee07492014bfc2d9f11a97c4f3d3940cb2acb1f6705a`
- FWSC SHA-256: `6ff09ea142a48f9a3c819a5316befa3d4131a3a69efaabb59253fa34b8678e69`
- source archive SHA-256: `49dfc9a52b070cc2a621192d91d67c5f9c165c3f705b388434592c55bbc23c7e`
- package identity: `FM-1_20000401`
- host UI/persistence, sequencer, engine, factory bank, Rack SysEx and bank tooling tests pass
- clean source rebuild reproduces the app and FWSC exactly

Hardware testing is still required.
