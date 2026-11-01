# FM1 Bass Station TEST11 — exact-time PORTA ramp and factory-state cross-checks

## Purpose

TEST11 continues directly from TEST10. The recovered 100-program factory bank,
128-slot persistent USER / COMMUNITY bank, Rack/native/VST2 import paths and
flash object layout are retained. This release concentrates on the remaining
PORTA timing mismatch and on gathering more evidence for the still-unknown
original MIX / pulse-width panel transfer curves.

## Timed PORTA correction

TEST10 fixed *when* PORTA is allowed: SINGLE and MULTI can glide generally,
while AUTO is legato-only. However, pitch movement still used a one-pole RC
smoother. With a nominal 1.0 second glide, that smoother moved only about 63%
of the way toward the destination after one second and approached the target
asymptotically.

Historical Bass Station / Bass Station Rack documentation specifies PORTA as a
0..5 second **ramp time**, while the software manual describes the control in
terms of the time taken for one note to reach the next. TEST11 therefore replaces
the RC smoother with a true timed pitch ramp.

Implementation details:

- glide is linear in pitch / semitone space, matching a constant control-voltage
  style ramp rather than a linear-Hz sweep;
- runtime pitch uses Q16.16 MIDI-note units;
- a small integer DDA distributes fractional steps exactly over the requested
  sample count;
- the destination is reached on the exact requested sample;
- no additional per-sample exp/log operation is introduced — the oscillator's
  existing pitch conversion is reused;
- changing PORTA to effectively zero during a glide snaps to the destination.

At 44.1 kHz, a 1.0 second C-to-C octave glide is regression-tested to be exactly:

- 44,100 samples total;
- +6 semitones after 22,050 samples;
- +12 semitones / destination after 44,100 samples.

The same exact-time regression is applied in the descending direction.

## Trigger-mode behavior retained

- SINGLE: single-trigger envelope behavior; PORTA glides between note targets,
  including separated notes when a previous pitch exists.
- MULTI: each new note-on retriggers the envelopes; PORTA still glides pitch.
- AUTO: legato-only glide; a fresh phrase after all keys are released starts on
  the destination pitch immediately.
- MIDI note 0 is explicitly regression-tested as a valid glide origin.

## MIX / pulse-width fidelity investigation

TEST11 adds a second independent official Novation Automap factory-program
anchor: preset 13/100, `TB303 Eow Bass` (recovered factory index 12).

Recovered/native values cross-check against the screenshot include:

- OSC1 PW raw `0x03` -> Automap display `-58`;
- MIX raw `0x4D` -> Automap display `-64`;
- LFO speed raw `0x9C` -> display `78`;
- LFO delay raw `0x80` -> display `64`;
- filter frequency raw `0x58` -> display `44`;
- resonance raw `0xFF` -> display `127`.

Together with factory 00 `Moog Bass` (MIX raw `0xAF` -> display `3`, OSC1 PW
raw `0x03` -> display `-58`), this proves that the original software MIX/PW
presentation is not recoverable by simply reinterpreting the native byte as a
linear signed panel value.

TEST11 deliberately does **not** invent a polynomial or piecewise response from
two factory points. The recovered bytes and existing FM1 normalized control path
remain unchanged until we obtain a direct desktop plug-in sweep/A-B capture or
additional reliable anchors. See `research/TEST11-MIX-PW-ANCHORS.md`.

## Preset / storage compatibility

`bassstation_params_t` remains exactly 164 bytes in both TEST10 and TEST11 and no
parameter fields moved or changed. The larger runtime `bassstation_t` state is
not persisted. Existing TEST9/TEST10 USER slots therefore require no migration.

The complete 100-program recovered factory bank is unchanged.

## Validation

- DSP ASan/UBSan/float-cast-overflow stress: passed.
- Exact 1.0 s ascending PORTA regression: passed.
- Exact 1.0 s descending PORTA regression: passed.
- 0.5 s SINGLE and AUTO glide scheduling regressions: passed.
- AUTO non-legato direct-pitch regression: passed.
- MIDI-note-0 glide-origin regression: passed.
- TEST10 envelope timing regressions: passed.
- TEST10 MIDI-clocked LFO idle/Stop/loss regressions: passed.
- Rack SysEx regression: passed.
- 100-program factory decode sanitizer: passed.
- New official `TB303 Eow Bass` factory anchors: passed.
- VST2/ZIP/multi-batch bank-preparation tests: passed.
- Host UI/MIDI smoke across all 13 pages: passed.
- USER slot 128, native import and factory-bank host regressions: passed.
- Clean target rebuild from the bundled toolchain: byte-identical.

## Hardware test focus

1. Set PORTA to about 1 second and play an octave. TEST11 should audibly arrive
   on the destination at the selected time rather than still slowly approaching
   it as TEST10 did.
2. Try both upward and downward large intervals; their elapsed glide time should
   match.
3. In SINGLE, test both overlapping and separated notes: the pitch should glide
   while the envelope retains single-trigger behavior for the held phrase.
4. In AUTO, legato notes should glide; release all keys and the next note should
   start directly on pitch.
5. Re-check a few familiar factory programs and saved USER slots to confirm the
   sound/storage changes are isolated to PORTA movement.

## Target resources

- Application: **104,648 / 581,564 bytes** (476,916 bytes free)
- Main RAM (.data + .bss): **22,680 / 98,304 bytes** (75,624 bytes free)
- Existing large-buffer pool: **69,124 / 344,064 bytes** (274,940 bytes free)
- Software-double helpers: **none**

Compared with TEST10, TEST11 is **164 bytes smaller in application flash** and
uses **32 bytes more main RAM** for the exact-timing runtime glide state. No
persistent parameter/storage size changed.

## Reproducibility

A clean rebuild after deleting `app/build`, `release`, and the extracted
`build-toolchain` produced byte-identical outputs:

- ELF SHA-256: `565e4e32e2c37c06e820cd64a4a2381cfa3a4b7efeff5c49fd9c10b611787a70`
- app BIN SHA-256: `5445658377e519909f1f589b9987f680e8a0645bba162c9d972d24c47185d361`
- FWSC SHA-256: `5ec341dff4d861a93e9e05dd02c31274219f617420c6a71689829c72007c36ee`
- M-Flasher SHA-256: `86330ea2ff2b553886ae53db80498dcb97edfa0fecda395c531cc42f9bed0a87`
