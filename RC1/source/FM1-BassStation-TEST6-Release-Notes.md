# FM1 Bass Station TEST6 — plug-in behavior correction pass

## Purpose

TEST6 continues directly from TEST5. It does not change the monophonic architecture or the Rack SysEx format. This pass corrects several software-plug-in behaviors against the Bass Station plug-in user manual and fixes one value in the verified factory `Moog Bass` program.

## Changes from TEST5

### Software envelope range

The software-panel conversion for both envelopes now reaches the documented **20 second** maximum for attack, decay and release. TEST5 still used shorter legacy conversion ceilings for incoming plug-in MIDI CC/factory values even though the editable FM1 parameter range was wider.

### LFO saw direction

The plug-in's saw LFO is a **rising saw**. TEST5 generated a falling saw. TEST6 reverses that waveform and the FM1 LFO page now labels it `SAW UP`.

### Aftertouch / Breath PITCH routing

The plug-in manual describes the PITCH controls in the Aftertouch and Breath sections as control of **LFO vibrato depth**. TEST5 incorrectly added those controller values as a static pitch offset. TEST6 routes them into oscillator LFO pitch depth, matching the Modwheel PITCH behavior.

Filter and amplitude-bias pressure/breath routes are unchanged.

### Verified Moog Bass correction

The recovered `Moog Bass` factory-program record specifies Modwheel PITCH = **5**. TEST5 loaded 4. TEST6 corrects the verified factory program to 5 and adds a regression assertion for it.

## Compatibility

- TEST5 patch/settings storage layout is unchanged, so the existing `BST5` storage magic is intentionally retained.
- The 76-byte Bass Station Rack single-program SysEx import/export format is unchanged.
- Free-running software LFO remains capable of approximately 1 kHz.
- Rack-specific range clamps remain separate from the wider software-panel path.

## Validation

- Host UI/MIDI smoke: **PASS** across all 13 pages.
- Verified `Moog Bass` wheel-pitch assertion: **PASS**.
- 20-second envelope endpoint via documented CC lanes: **PASS**.
- 1 kHz software LFO CC assertion: **PASS**.
- ASan/UBSan/float-cast-overflow DSP stress: **PASS**.
- Rising-saw LFO regression: **PASS**.
- Aftertouch PITCH static-offset regression: **PASS**; with zero LFO value, enabling pressure pitch produces byte-identical audio.
- Bass Station Rack SysEx round-trip regression: **PASS**.
- Clean target rebuild: **byte-identical**.
- Target audit: no software-double helpers.

## Target resources

- Application: **95,488 / 581,564 bytes**
- Main RAM (.data + .bss): **18,968 / 98,304 bytes**
- Existing large-buffer pool: **69,124 / 344,064 bytes**

## Factory-bank status

Only program 00, **Moog Bass**, currently has a verified plug-in parameter record. The complete 100 factory names remain preserved under `research/`, but TEST6 does not invent parameter values for the remaining programs.

The first 40 software factory names align with the original Bass Station Rack factory-program inventory, which remains a useful recovery lead, but no unverified Rack values have been promoted into the FM1 bank.

## Hardware status

TEST6 has been built and host/sanitizer validated, but has **not** been flashed or listened to on an FM1 by the assistant. The preferred hardware test is the `.fwsc` application update.
