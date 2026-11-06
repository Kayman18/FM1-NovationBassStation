# Novation Bass Station Rack single-patch SysEx map used by TEST8

TEST8 supports the 76-byte Bass Station Rack single-program frame:

`F0 00 20 29 01 03 22 ... F7`

The application receives the 74 bytes between `F0` and `F7`. Legacy 8-bit values
are normally transmitted as high/low nibble pairs. The two packed switch bytes
are nibble-expanded on the wire as well; TEST8 explicitly decodes/encodes them
that way rather than treating bytes 71/73 as literal packed values.

Implemented fields include portamento; OSC2 fine/semitone; oscillator mix;
OSC1/OSC2 ENV/LFO pitch depths; independent PWM amounts and sources; LFO
rate/delay/shape/clock division; filter cutoff/resonance/ENV/LFO depths/slope;
both ADSRs and velocity depths; mod-wheel pitch/filter depths; Breath and
Aftertouch pitch/filter/EG-bias depths; bend range; oscillator waves/range;
oscillator sync; and trigger mode.

## Software-vs-Rack boundaries

The Bass Station software plug-in is deliberately wider than the Rack in several
places. TEST8 preserves the plug-in/native range internally and clamps only at
the Rack compatibility boundary:

- plug-in OSC2 SEMI: **0..+12 semitones**
- plug-in OSC2 DETUNE: **-96..+96 cents**
- Rack OSC2 SEMI: **0..+12 semitones**
- Rack OSC2 fine/detune: **about -50..+50 cents**
- plug-in envelope edit path: up to approximately **20 s**
- Rack envelope conversion: the shorter hardware-compatible ranges
- Rack mod-wheel pitch depth remains its native **0..15** value
- Rack pitch-bend range remains **0..12 semitones**
- Rack Breath/Aftertouch pitch/filter depths remain **-7..+7**

Rack import does not overwrite software-only master tune, master level, or the
FM1 filter key-tracking policy because those fields are not represented in the
single-program Rack frame.

The Rack filter-mod source selector bit is decoded for compatibility but does
not replace the plug-in model's simultaneous ENV and LFO filter depth values.
