# FM1 Bass Station TEST9 — 128 USER slots + VST2 FXP/FXB bank bridge

## Purpose

TEST9 continues directly from TEST8. The recovered 100-program factory bank and
sound engine are intentionally left intact while preset interoperability is
expanded for the legacy user/community banks requested during testing.

## 128-slot USER / COMMUNITY bank

- USER storage expands from 100 to **128 persistent native patches**.
- All 128 slots are addressable by MIDI Program Change 0..127 when USER is active.
- Local PRESET navigation wraps across all 128 USER slots.
- Rack SysEx capture, native plug-in-record import, load/save/name/clear and reboot
  persistence all support slots 0..127.
- Existing TEST7/TEST8 slots 0..99 keep their original object IDs and flash
  addresses, so previously saved USER patches remain compatible.
- Two new torn-write-safe A/B objects occupy unused app-data sector pairs at
  `0xCE000` and `0xD0000`; the update staging area is untouched.

The 128-slot size is deliberate: it is the largest bank that still maps directly
to one standard MIDI Program Change byte and one 7-bit-clean SysEx slot byte.

## VST2 FXP / FXB import

`tools/prepare_bassstation_bank.py` now understands standard VST2 containers:

- `CcnK` root container
- `FPCh` opaque program chunk
- `FBCh` opaque bank chunk
- Bass Station IDs `Nvb2` and legacy `NvB2`

Opaque chunks are then validated for the recovered classic `tffp` Bass Station
state. Regular parameter-only `FxCk` / `FxBk` files are recognized but are not
silently guessed when they do not contain the classic state.

ZIP containers, raw Rack SysEx and Standard MIDI Files remain supported.

## Collections larger than 128 patches

The converter no longer aborts just because an archive is larger than the FM1
USER bank. It automatically creates numbered load sets of up to 128 patches:

- `...-01.mid/.syx/.txt`
- `...-02.mid/.syx/.txt`
- etc.

This is specifically useful for legacy archives containing more than one bank or
more than 128 individual patches. Every unique compatible patch is preserved in
the manifests rather than silently discarded.

## MIX / pulse-width research

Additional host mapping evidence confirms that MIX and both PULSE WIDTH controls
are bipolar at the original plug-in surface: exact centre is normalized 64/127
and displayed as `0`. This is useful confirmation, but it still does not prove
the proprietary plug-in's internal transfer/taper between panel position and the
compact stored byte. TEST9 therefore **does not change the audio DSP or factory
bank conversion for those controls**. A guessed curve would be less faithful
than retaining the recovered state until desktop A/B evidence is available.

## Validation

- Host UI/MIDI smoke across all 13 pages: passed.
- New USER slot 127 (display slot 128) native import/save/reboot/load: passed.
- MIDI Program Change 127 in USER bank: passed.
- Existing USER slots, factory bank and Moog Bass anchors: passed.
- Synthetic standards-compliant `FPCh` and `FBCh` Bass Station wrappers: passed.
- Non-Bass-Station VST2 ID rejection: passed.
- Mixed Rack + VST2 ZIP conversion: passed.
- 136-patch conversion split into 128 + 8 load sets: passed.
- DSP ASan/UBSan/float-cast-overflow stress: passed.
- Rack SysEx regression: passed.
- 100-program factory decode sanitizer: passed.

## Hardware test focus

Confirm that USER patches saved under TEST8 still appear in slots 1..100. Then
exercise a high slot such as USER 128: load/import it, SAVE, power-cycle, and
confirm it returns. Factory sound should be unchanged from TEST8.

## Fidelity boundary

TEST9 does not claim bit-exact proprietary Novation DSP. The exact original MIX
and PULSE WIDTH transfer/taper remains a desktop A/B target; no speculative DSP
change was made in this release.

## Target resources

- Application: **104,708 / 581,564 bytes** (476,856 bytes free)
- Main RAM (.data + .bss): **22,648 / 98,304 bytes** (75,656 bytes free)
- Existing large-buffer pool: **69,124 / 344,064 bytes** (274,940 bytes free)
- Software-double helpers: **none**

The two extra USER-bank chunks consume flash data sectors only; runtime bank-cache
RAM is unchanged because one 20-patch chunk is cached at a time.

## Reproducibility

After deleting `app/build`, `release`, and the extracted `build-toolchain`, a
self-contained rebuild from this source kit reproduced byte-identical ELF,
application, `.fwsc`, and M-Flasher outputs.
