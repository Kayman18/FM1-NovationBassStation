# FM1 Bass Station TEST4 — plug-in panel fidelity + Rack SysEx patch bridge

## Purpose
TEST4 reshapes the TEST3 engine/control model around the documented Bass Station 2.2 software plug-in rather than the earlier hybrid diagnostic layout. Audio remains monophonic, full-rate 44.1 kHz with the 2x nonlinear filter introduced in TEST2.

## Fidelity changes
- OSC1 is fixed at 8-foot pitch, matching the software plug-in panel.
- OSC2 range remains 16'/8'/4'/2'.
- OSC2 semitone is now -12..+12.
- OSC2 fine tuning is now +/-96 cents internally.
- Master tune added at +/-50 cents.
- Independent PWM source per oscillator: MANUAL / ENV2 / PWM LFO.
- Mixer is now the software plug-in's single OSC1<->OSC2 crossfade rather than independent oscillator/noise levels.
- Filter has simultaneous ENV2 and LFO modulation depths rather than one source selector.
- Envelope edit range extends to about 20 seconds for the software-panel path.
- Existing 12/24 dB behavior, MIDI-clock LFO, trigger modes, velocity, Aftertouch/Breath and 2x filter are retained.

## Bass Station Rack patch compatibility
TEST4 adds a complete codec for the 76-byte Novation Bass Station Rack single-patch SysEx frame (`F0 00 20 29 01 03 22 ... F7`). Incoming Rack single-patch dumps over USB MIDI load directly into the FM1. A development request `F0 7D 42 53 04 01 F7` asks the FM1 to transmit its current patch as a Rack-format frame.

The codec maps oscillator waves/range, independent PWM, sync, trigger mode, filter slope and modulation, both envelopes, LFO/clock, wheel, bend, Aftertouch and Breath. Software-only ranges are safely clamped on Rack export.

## Factory bank status
The official manual's 100 factory program names have been recovered and are preserved in `research/FACTORY-PROGRAM-NAMES.txt`. The actual 100 software preset value records are not yet claimed recovered, so TEST4 deliberately retains the five diagnostic patches rather than inventing settings.

## Validation
- ASan/UBSan/float-cast-overflow engine stress passes across waves, slopes, independent PWM sources, +/-12 semitone, +/-96 cent fine tune, +/-50 cent master tune, dual filter modulation and controller extremes.
- Rack SysEx encode/decode regression passes, including packed mode bits and Rack range clamps.
- Host UI/MIDI smoke passes all 13 pages, independent PWM controls, diagnostic presets, local keyboard, USB MIDI, sustain and legacy Rack CC automation.
- Host render remains about 0.4% of real time on the validation machine (directional only).
- Target build has no software-double helpers.

## Target resources
- Application: 95,320 / 581,564 bytes
- Main RAM: 18,968 / 98,304 bytes
- Large-buffer pool: 69,124 / 344,064 bytes

## Hardware test
Listen to BASS / SYNC / PWM / REZ again, especially independent PWM and filter ENV+LFO interaction. PERFORMANCE should remain far below deadline. If you have a Bass Station Rack single-patch SysEx dump, TEST4 can now receive it directly over USB MIDI.
