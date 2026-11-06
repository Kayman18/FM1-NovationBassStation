# Bass Station plug-in factory-bank recovery: program 00 `Moog Bass`

TEST5 first reconstructed program 00 **Moog Bass** from a Novation Automap
control-map screenshot. TEST8 subsequently recovered the complete original
100-program Bass Station plug-in state bank and uses Moog Bass as an independent
cross-check of the decoder.

## Independent Automap anchor

The Novation Automap view identifies `Bass Station - Novation DMS`,
`Preset 1/100: Moog Bass` and exposes the plug-in's displayed settings. Useful
independent anchors include:

- trigger MULTI
- OSC1 sawtooth
- OSC1 ENV 1 / LFO -3 / PW -58 / PWM source LFO
- OSC2 square, range 8', semitone 0, detune 7
- OSC2 ENV 0 / LFO -1 / PW -57 / PWM source MANUAL
- oscillator sync OFF and mixer display 3
- LFO speed 79, delay 91, triangle
- cutoff 1, resonance 40, 24 dB, filter ENV +52, filter LFO -5
- AMP ADSR 5 / 32 / 126 / 61, velocity 1
- MOD ADSR 25 / 94 / 15 / 49, velocity 64
- mod-wheel pitch 5, filter 0; pitch bend 2
- master level 63, portamento 1, master tune 0

The TEST8 compact program-00 record is:

`03 8F 03 AF 82 7B 03 9E B6 03 50 77 0B 40 FC 7B 03 32 BC 1F 63 81 80 7E 05 E9 05 00 00 00 02 00 A9 01`

Many Automap integer values are recovered directly as half-scale values from
this native 8-bit record (for example 79/91/40 and the ADSR values), while the
packed controller/switch fields reproduce wheel-pitch 5, bend 2 and switch bytes
`A9 01`. This is independent evidence that the 34-byte records are the intended
factory parameter state rather than a names-only inventory.

## Software-range correction retained in TEST8

The original software plug-in manual documents OSC2 DETUNE at **+/-96 cents**.
The v2.2 UI XML and original front-panel scale define OSC2 SEMI as **0..+12 semitones**; earlier FM1 notes had misread the manual's "up/down by a full octave" wording as a signed range. Rack SysEx uses the same 0..+12 semitone range but retains the narrower roughly +/-50-cent fine-tune path.

## Complete bank source

`research/BassStationStereo-Factory.pchk` is the recovered classic plug-in state
used to generate the firmware table.

SHA-256:

`82b6c04d602662d2d3745ec7f3208a90cf03f6db7df481b63dbc998afecd13df`

The same factory naming is independently visible in the public
`jhorology/nks-presets-collection` BassStationStereo metadata, where the presets
are labelled `BassStation Factory`.
