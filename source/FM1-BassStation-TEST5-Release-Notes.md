# FM1 Bass Station TEST5 — first verified factory patch + complete plug-in CC map

## Purpose

TEST5 continues from TEST4 without changing the mono/full-rate architecture. It
uses recovered Novation documentation to add the first factory program with
verified parameter values, expands the software LFO range, fixes the missing Rack
clock division, and fills out the documented Bass Station plug-in MIDI CC map.

## Factory program recovery

Factory program slot 1 on FM1 is now the original Bass Station plug-in program
00, **Moog Bass**. Its parameter values were recovered from a Novation Automap
mapping screenshot that shows the plug-in at `Moog Bass`, `1/100`, together with
all mapped values.

TEST5 converts those displayed plug-in values through a separate software-panel
conversion layer. They are not confused with the narrower Bass Station Rack
single-program SysEx ranges.

The remaining 99 factory program names are preserved under `research/`, but their
settings are not invented or enabled yet.

## LFO / clock corrections

- Free-running plug-in LFO range is expanded to the documented software range up
  to approximately **1 kHz**.
- Rack SysEx keeps the narrower hardware-compatible LFO conversion range.
- The Rack MIDI-clock table now contains all 16 non-free divisions; the previously
  omitted raw value **64** is present as the 1-bar-triplet entry.
- FM1 LFO page exposes FREE plus all 16 divisions.

## MIDI CC automation

TEST5 implements the documented Bass Station plug-in CC lanes used by the current
engine: portamento, master level, LFO rate/delay/sync, OSC1/OSC2 ENV/LFO pitch,
OSC1/OSC2 pulse width, OSC2 semitone/fine, oscillator mix, filter LFO/ENV depths,
cutoff, resonance, and both envelope ADSR/velocity controls.

Existing CC1 mod wheel, CC2 breath, CC64 sustain and all-notes-off handling are
retained.

## Rack SysEx

The TEST4 76-byte Bass Station Rack single-program import/export remains present.
TEST5 corrects the clock-division table while retaining the software-vs-Rack
range clamps documented in `research/RACK-SYSEX-MAP.md`.

## Validation

- ASan/UBSan/float-cast-overflow engine stress passes with the free LFO driven at
  1 kHz and the full MIDI-clock range exercised.
- Rack single-program SysEx round-trip passes and explicitly verifies raw clock
  value 64.
- Host UI/MIDI smoke passes all 13 pages, preset wrap, local keyboard audio,
  USB MIDI, sustain, pressure/breath, documented CC edits, verified Moog Bass
  slot values, and CC16 reaching the >900 Hz LFO region.
- Host render remains about 0.5% of real time on the validation machine; this is
  directional only and not an FM1 CPU measurement.
- Target audit finds no software-double helpers.

## Target resources

- Application: **95,392 / 581,564 bytes**
- Main RAM (.data + .bss): **18,968 / 98,304 bytes**
- Existing large-buffer pool use: **69,124 / 344,064 bytes**

## Current boundary

TEST5 does **not** claim the complete 100-program plug-in bank. Only Moog Bass has
verified settings. The remaining names are research inventory while value
recovery continues.
