# TEST11 USER / COMMUNITY bank

TEST11 retains 128 persistent USER slots, separate from **INIT + 100 recovered
factory programs**. Each USER slot stores the native FM1 Bass Station parameter
structure, so imported plug-in patches can keep software-only ranges such as the
20 s envelopes and ~1 kHz free LFO.

## FM1 controls

- `REC` toggles **BUILT-IN** / **USER** banks.
- `PRESET` selects INIT + Factory 00..99 or USER 1..128.
- `SAVE` writes the current patch to the selected USER slot.
- MIDI Bank Select MSB CC0: `0` = BUILT-IN, non-zero = USER.
- BUILT-IN Program Change 0..99 selects Factory 00..99; INIT is a local-only
  extra selection. Program Change 100..127 is ignored in BUILT-IN because no
  factory programs exist at those numbers.

An empty USER slot loads INIT and displays `EMPTY`; editing and pressing SAVE
creates the slot.

## Rack SysEx bank capture

TEST9 accepts the historical 76-byte Bass Station Rack single-program frame and
retains the TEST7 development controls:

- `F0 7D 42 53 07 20 ss F7` — begin Rack capture at slot `ss` (0..127)
- `F0 7D 42 53 07 21 F7` — stop capture
- `F0 7D 42 53 07 22 ss F7` — export USER slot as a Rack patch
- `F0 7D 42 53 07 23 ss F7` — load USER slot
- `F0 7D 42 53 07 24 ss F7` — save current patch to USER slot
- `F0 7D 42 53 07 25 ss <ASCII> F7` — set USER slot name (19 chars max)
- `F0 7D 42 53 07 26 ss F7` — clear USER slot

TEST9 corrected the Rack switch-byte wire packing using historical editor source.
Rack export necessarily clamps software-only ranges to the older Rack limits.

## Native classic plug-in record import

To avoid those Rack clamps, TEST9 adds an explicit 34-byte compact plug-in record
transport. Each byte is nibble-expanded to remain MIDI/SysEx 7-bit clean:

`F0 7D 42 53 08 30 ss <68 nibbles> F7`

This is the path used by the bank-preparation utility for classic `tffp` records.
It decodes directly to the software-panel parameter ranges before storing the USER
slot.

## Preparing old/community banks

`tools/prepare_bassstation_bank.py` is dependency-free and accepts:

- raw `.syx` / binary data containing Rack 76-byte program dumps,
- Standard MIDI Files containing those dumps,
- classic Bass Station plug-in states containing `tffp` (including `.pchk` and
  common VST2 `.fxb` / `.fxp` wrappers), and
- `.zip` archives containing any of the above.

It outputs a paced Type-0 MIDI import, a concatenated SysEx stream, and a manifest.
Rack patches are captured one slot at a time; native plug-in records carry an
explicit destination slot, so mixed sources cannot overwrite each other.

Example:

    python3 tools/prepare_bassstation_bank.py old_bank.zip another.fxb \
      --start 0 -o FM1-Community-Bank

For a `.pchk` or individual `.fxp`, the selected program is imported. A bank
container carrying a full `tffp` bank imports all 100 programs, subject to the
available USER-slot count.

## TEST9 VST2 and large-bank conversion

TEST9 recognizes the standard VST2 `CcnK` container and the opaque chunk
forms `FPCh` (program) and `FBCh` (bank). It accepts the classic Bass Station
plug-in IDs `Nvb2` and legacy `NvB2`, then scans the opaque plug-in chunk for
the verified `tffp` state rather than treating arbitrary `.fxp`/`.fxb` bytes
as a patch. Parameter-only `FxCk`/`FxBk` files are recognized but deliberately
not guessed when no classic Bass Station state is present.

The FM1 USER bank now exposes slots 1..128, matching the full MIDI Program
Change range. If an input collection contains more than 128 unique compatible
patches, `prepare_bassstation_bank.py` writes numbered load sets (`-01`, `-02`,
etc.) instead of dropping data. Existing TEST7/TEST8 USER slots 1..100 keep the
same flash objects and addresses.
