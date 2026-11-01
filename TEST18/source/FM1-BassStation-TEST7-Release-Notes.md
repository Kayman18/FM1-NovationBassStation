# FM1 Bass Station TEST7 — persistent USER / COMMUNITY bank

## Purpose

TEST7 continues from TEST6 without changing the monophonic audio architecture.
Its main goal is to make recovered and community Bass Station patches practical
to use on FM1 without mixing unverified material into the factory bank.

## USER / COMMUNITY bank

- Adds **100 persistent USER slots**, completely separate from the built-in bank.
- Each USER slot stores the full native FM1 Bass Station parameter structure,
  preserving software-only ranges such as 20-second envelopes and the ~1 kHz LFO.
- USER patches have persistent names (19 characters max).
- Empty slots are deterministic INIT patches and display `EMPTY`.
- `REC` toggles **BUILT-IN / USER**.
- `PRESET` selects within the current bank.
- `SAVE` writes the current patch to the selected USER slot.
- MIDI CC0 Bank Select MSB: `0` selects BUILT-IN; non-zero selects USER.
- MIDI Program Change selects the program within the current bank.

## Rack SysEx bank capture

Normal 76-byte Bass Station Rack single-program import/export remains supported.
TEST7 adds a small development SysEx control protocol using the non-commercial
manufacturer ID `0x7D` so a stream of Rack single-program dumps can be captured
straight into sequential USER slots.

The full command list is in `research/USER-BANK-IMPORT.md`.

## Bank preparation tool

`tools/prepare_bassstation_bank.py` scans `.syx`, Standard MIDI Files, and other
binary containers for complete original Bass Station Rack single-program frames.
It can generate:

- a paced import `.mid`,
- a concatenated `.syx` stream, and
- a manifest of source/name/slot assignments.

The default MIDI spacing is 500 ms so FM1 has time to commit each patch to flash.
The converter intentionally does **not** guess at arbitrary VST2 `.fxb/.fxp`
chunks until the original plug-in serialization/order is verified.

## Storage / compatibility

- USER bank storage uses five independent A/B-protected flash objects, 20 patches
  each.
- The update staging area is left untouched.
- Existing settings/panel storage locations remain compatible.
- TEST6 settings magic is recognised and migrated forward rather than discarded.

## Factory and community-bank status

The built-in verified factory content remains conservative: **Moog Bass** is the
only original software factory program whose complete settings are currently
claimed verified. The remaining factory names remain research inventory.

Public sources for additional original Bass Station banks were located during
TEST7 research, including a 7-bank / 108-single-patch archive and official
Novation restore material. No third-party patch archive is redistributed in
TEST7 because the available archive provenance/redistribution terms were not
clear enough, and the binary files were not available to the build environment
for validation. TEST7 provides the import path for those banks once their actual
bytes are obtained and checked.

## Validation

- Host UI/MIDI smoke: passed all 13 pages and both banks.
- USER save/load persistence across simulated reboot: passed.
- Sequential Rack SysEx capture to USER slots: passed.
- CC0 bank select + Program Change: passed.
- DSP ASan/UBSan/float-cast-overflow stress: passed.
- Rack SysEx sanitizer/regression: passed.
- Bank-preparation utility regression: passed.
- Clean self-contained target rebuild: byte-identical.
- Target audit: no software-double helpers.

## Target resources

- Application: **97,564 / 581,564 bytes**
- Main RAM (.data + .bss): **22,648 / 98,304 bytes**
- Existing large-buffer pool use: **69,124 / 344,064 bytes**

TEST7 adds roughly 3.7 KB of main-RAM bank cache while retaining very large
application and pool headroom.

## Hardware test focus

1. Confirm TEST6 patches still sound unchanged.
2. Press `REC` to enter USER bank; verify empty slots display `EMPTY`.
3. Edit an empty USER slot, press `SAVE`, move away and back, and confirm it reloads.
4. Power-cycle FM1 and confirm the saved USER patch/name persists.
5. If a Rack-format bank is available, use the generated paced `.mid` import and
   verify sequential USER slots populate without audio/flash instability.
