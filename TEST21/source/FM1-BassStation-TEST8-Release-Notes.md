# FM1 Bass Station TEST8 — complete 100-program factory bank

## Purpose

TEST8 is the factory-bank recovery build. It retains TEST7's persistent
100-slot USER / COMMUNITY bank and replaces the previous partial/diagnostic
built-in content with the complete recovered **100-program Bass Station software
plug-in factory bank**.

## Complete factory bank

- Recovered all 100 compact 34-byte factory program records plus all 100 names
  from a classic `BassStationStereo` plug-in state.
- `INIT` remains an extra local starting patch; FACTORY programs 00..99 follow it.
- MIDI Program Change 0..99 selects factory programs 00..99 in the built-in bank.
- The factory state is retained under `research/` with a deterministic extractor
  that regenerates `app/factory_bank.h`.
- Program 00 `Moog Bass` is independently cross-checked against Novation's
  Automap control-map documentation. The recovered record reproduces the shown
  oscillator/filter/envelope/controller values and packed mode bits.

## Plug-in fidelity / Rack compatibility corrections

- Software OSC2 SEMI now retains the plug-in's documented **-12..+12 semitone**
  range.
- Software OSC2 DETUNE now retains the plug-in's documented **+/-96 cent** range.
- Rack SysEx remains intentionally hardware-compatible at **0..+12 semitones**
  and roughly **+/-50 cents**, with safe clamping on export.
- Rack mod-wheel pitch depth is preserved as its native 0..15 value.
- Corrected the Rack codec's packed switch handling: the two packed switch bytes
  are themselves high/low-nibble expanded in the 76-byte wire frame.
- The Rack clock table retains all 16 non-free divisions, including raw 64.

## USER / COMMUNITY bank

TEST7's 100 persistent native USER slots remain intact. `REC` switches banks and
`SAVE` writes the selected USER slot. Sequential Rack SysEx capture, CC0 bank
selection, Program Change, export/load/clear development commands and reboot
persistence are retained.

`tools/prepare_bassstation_bank.py` accepts compatible Rack SysEx material and
classic Bass Station `tffp` plug-in states. Community material is kept separate
from the FACTORY bank. Online bank leads are documented under `research/`; their
archive bytes are not falsely claimed as bundled when the binary download bridge
cannot materialize them into the build container.

## Validation

- Host UI/MIDI smoke across all 13 pages: passed.
- Factory-bank traversal, first/last program and Moog Bass anchors: passed.
- 100-program factory decode sanitizer stress: passed.
- DSP ASan/UBSan/float-cast-overflow stress: passed.
- Rack single-program SysEx round-trip, packed switches and raw clock 64: passed.
- USER bank store/load/reboot and sequential import regression: passed.
- Bank-preparation utility tests: passed.
- Target audit: no software-double helpers.

## Fidelity boundary

TEST8 recovers authentic stored factory parameter data but does not claim
bit-exact proprietary Novation DSP. The exact original plug-in transfer/taper for
MIX and PULSE WIDTH remains a desktop A/B/listening refinement target; those
controls are not reverse-engineered beyond what is supported by the recovered
state and public documentation.

## Hardware test focus

Cycle through a wide range of factory sounds, especially programs using sync,
PWM, large positive/negative OSC2 tuning and extreme envelopes. Confirm USER bank
save/load still survives a power cycle. PERFORMANCE counters should remain well
below the deadline as in prior builds.

## Target resources

- Application: **104,676 / 581,564 bytes** (476,888 bytes free)
- Main RAM (.data + .bss): **22,648 / 98,304 bytes** (75,656 bytes free)
- Existing large-buffer pool: **69,124 / 344,064 bytes** (274,940 bytes free)
- Software-double helpers: **none**

A clean rebuild after deleting `app/build`, `release`, and the extracted
`build-toolchain` produced byte-identical ELF, application, `.fwsc`, and
M-Flasher outputs.

## Current boundary

Hardware listening is still required. TEST8's recovery claim concerns the stored
factory program state and verified parameter decoding; it does not claim
bit-exact proprietary Novation oscillator/filter/DSP behavior.
