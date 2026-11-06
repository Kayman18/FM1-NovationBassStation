# FM1 Bass Station TEST10 — envelope timing, PORTA modes, clock-idle LFO

## Purpose

TEST10 continues directly from TEST9. The recovered 100-program factory bank,
128-slot persistent USER / COMMUNITY bank, Rack/native/VST2 import paths and
flash object layout are retained. This release concentrates on three sound-engine
behaviors found during a fresh comparison against the published Bass Station
plug-in operating description.

## Envelope stage-time correction

The previous DSP used the displayed attack/decay/release seconds directly as a
one-pole RC time constant. That preserved an exponential contour, but the stage
completion point occurred several time constants later: a nominal 1 s attack took
about 4.5 s and a nominal 20 s attack could take roughly 90 s.

TEST10 keeps the same exponential envelope shape but calibrates the one-pole
coefficient to the engine's actual stage-completion thresholds. Direct host
measurement with the production fast-math path now gives:

- 1.0 s attack: ~1.0003 s
- 1.0 s release: ~1.0002 s
- 20.0 s attack: ~20.33 s
- 20.0 s release: ~19.94 s

The small 20 s attack offset is float/fast-exp approximation at an extremely
small per-sample coefficient; it is intentionally left as-is rather than adding
per-sample complexity.

## PORTA / trigger-mode correction

The published behavior treats PORTA as a general glide control, while AUTO is the
mode that restricts glide to legato playing. TEST9 instead forced SINGLE note
changes directly to the new pitch.

TEST10 now behaves as follows:

- SINGLE: envelope is single-trigger; PORTA glides between note targets, including
  a new phrase when a previous pitch exists.
- MULTI: every new note-on retriggers the envelopes; PORTA still glides pitch.
- AUTO: single-trigger behavior plus legato-only PORTA. A fresh note after all
  keys are released starts exactly on pitch.
- With PORTA effectively zero, all modes jump directly to the target pitch.

Held-note return and sustain handling remain last-note-priority as before.

## MIDI-clock-synced LFO idle behavior

When a clock division is selected, the published plug-in behavior says the LFO
has no effect while the sequencer is stopped or MIDI clock is not being received.
TEST9 froze the phase when clock stopped, but could leave a non-zero frozen LFO
value modulating pitch/filter/PWM.

TEST10 outputs zero modulation for a synced LFO when:

- no usable MIDI-clock period has been learned,
- MIDI Stop has been received, or
- incoming MIDI clock disappears for more than about four clock intervals.

The phase is frozen rather than reset, so clock resume continues coherently. LFO
delay/fade state still advances independently of the clock-output mute.

## Preset / storage compatibility

No parameter structure, factory-bank data or USER-bank flash object IDs changed.
TEST9 USER slots 1..128 remain compatible and no migration is required. The
recovered 100-program factory bank is unchanged.

## Community-bank work

The TEST9/TEST10 importer continues to support Rack SysEx, Standard MIDI Files,
classic `tffp` states, VST2 `FPCh`/`FBCh` FXP/FXB wrappers, ZIP archives and
multi-batch splitting over 128 patches. The Rekkerd and Rhythm Lab legacy-bank
leads remain documented in `research/ONLINE-BANK-LEADS.md`; archive bytes are not
claimed as bundled unless supplied locally and validated by the importer.

## Validation

- DSP ASan/UBSan/float-cast-overflow stress: passed.
- New 1 s envelope stage-time regression: passed.
- SINGLE PORTA regression, including a note gap: passed.
- AUTO legato glide and non-legato direct-pitch regressions: passed.
- Synced-LFO no-clock / Stop / clock-loss mute regressions: passed.
- Rack SysEx regression: passed.
- 100-program factory decode sanitizer: passed.
- VST2/ZIP/multi-batch bank-preparation tests: passed.
- Host UI/MIDI smoke across all 13 pages: passed.
- USER slot 128, native import and factory-bank host regressions: passed.
- Clean target rebuild from the bundled toolchain: byte-identical.

## Hardware test focus

1. Load a familiar long-envelope patch and compare attack/release timing to TEST9;
   TEST10 should be dramatically closer to the displayed seconds.
2. Set TRIGGER=SINGLE and a clearly audible PORTA time. Play separated and
   overlapping notes; pitch should glide while the envelope remains single-trigger
   during a held phrase.
3. Set TRIGGER=AUTO and PORTA. Legato notes should glide; after releasing all keys,
   the next note should start directly on pitch.
4. Set an LFO destination and a MIDI-clock division. Stop the external sequencer
   or remove clock: synced LFO modulation should become neutral rather than freeze
   at an arbitrary offset.
5. Confirm previously saved USER slots, especially slots 1..100 and 128, survive
   the update.

## Fidelity boundary

TEST10 does not claim bit-exact proprietary Novation DSP. The exact original MIX
and PULSE WIDTH panel-to-state transfer/taper remains a desktop A/B target; no
speculative DSP curve was introduced here.

## Target resources

- Application: **104,812 / 581,564 bytes** (476,752 bytes free)
- Main RAM (.data + .bss): **22,648 / 98,304 bytes** (75,656 bytes free)
- Existing large-buffer pool: **69,124 / 344,064 bytes** (274,940 bytes free)
- Software-double helpers: **none**

TEST10 adds only 104 bytes of application image versus TEST9 and adds no runtime
RAM or pool usage.

## Reproducibility

After deleting `app/build`, `release`, and the extracted `build-toolchain`, a
self-contained rebuild from this source kit reproduced byte-identical ELF,
application, `.fwsc`, and M-Flasher outputs.
