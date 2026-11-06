# FM1 Bass Station TEST14 release notes

TEST14 continues the native-state fidelity audit after TEST13. It corrects OSC2 SEMI across the software/native path.

## OSC2 SEMI correction

The original v2.2 decoder loads native record byte 2 directly as a normalized host value for parameter 8. The installer UI XML defines parameter 8 as integer `0..12`, and the original Bass Station front-panel scale is `0..+12`. Earlier FM1 builds incorrectly treated the compact byte as signed/two's-complement and exposed `-12..+12`.

TEST14 now uses the exact public integer mapping:

`semitone = round(raw * 12 / 255)`

Examples from the recovered factory bank:

- Yazoo Lead: raw `0xFC` -> `12` semitones (TEST13 could decode near 0)
- 92 Lead: raw `0x4F` -> `4` semitones
- Fuzz Bass: raw `0xFF` -> `12` semitones

The correction affects 26 of the 100 factory programs. Live CC23 and the FM1 OSC2 SEMI edit range are also now 0..+12. Rack SysEx was already 0..+12 and is unchanged.

TEST13's exact native MIX and PULSE-WIDTH transforms remain unchanged.

## Native trigger and LFO-sync encoding corrections

The software plug-in's second switch byte does **not** use the Rack SysEx trigger coding. The v2.2 DLL decodes native bits 4..5 as `0=MULTI`, `1=SINGLE`, and `2/3=AUTO`. TEST14 now uses that mapping only for native plug-in records; Rack SysEx keeps its independent `1=MULTI`, `2=SINGLE`, `3=AUTO` map. This corrects trigger mode on 13 recovered factory programs, including **TB303 Autoglide Bas**, **TB303 Eow Bass**, **Trance 1/2**, **Square PortaLead**, **92 Lead**, and several Porta/Lead/Bass programs.

The same DLL audit recovered the exact compact LFO-sync lookup: `0,2,3,4,6,8,9,12,16,18,24,32,36,48,72,96` map to public indices `0..15`. Native raw `64` is invalid/off, and the compact loader does not produce public enum 16. The built-in factory bank stores sync off in all 100 programs, so this part primarily improves imported native community patches.

## Native controller-depth packing correction

The original v2.2 loader packs PMod/FMod Breath and Aftertouch as two **unsigned 3-bit** values per byte. TEST13 and earlier treated those nibbles as signed 4-bit values. TEST14 now matches the DLL exactly: each field is masked with `0x7` and decoded as `0..7`; bit 3 is ignored. Native pitch-bend range is also read from the low nibble before its 0..12 clamp.

Rack SysEx remains deliberately separate: Rack Breath/Aftertouch pitch/filter depths retain their signed `-7..+7` interpretation. FM1 local editing follows the software plug-in's public `0..7` range; an imported Rack negative value remains valid until edited.

## Regression focus

- native semitone raw 0 / 127 / 255 -> 0 / 6 / 12
- public CC23 0 / 127 -> 0 / 12
- all 100 recovered factory records remain finite and in range
- TEST13 MIX/PW threshold/wrap vectors retained

## Validation and target resources

- engine ASan/UBSan/float-cast regression: PASS
- Rack SysEx signed-depth round-trip: PASS
- all 100 native factory records: PASS
- exact native switch/trigger/controller/LFO-sync adversarial vectors: PASS
- VST2 / ZIP / multi-batch bank preparation: PASS
- 13-page host UI/MIDI smoke: PASS
- corrected AUTO/SINGLE factory programs exercised through the host app: PASS
- clean target rebuild: byte-identical
- application: **104,944 / 581,564 bytes**
- RAM `.data + .bss`: **22,680 / 98,304 bytes**
- large-buffer pool: **69,124 / 344,064 bytes**
- software-double helpers: **none**

The persistent `bassstation_params_t` layout remains **164 bytes**, unchanged from TEST13, so no USER-bank migration is required.
