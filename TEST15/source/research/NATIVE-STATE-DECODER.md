# Bass Station v2.2 native 34-byte program decoder (TEST14)

TEST14 statically audited the original Novation Bass Station v2.2 x86 and x64
plug-in binaries from the official installer.  This file records the compact
program-state transformations used by the FM1 importer.  They are deliberately
kept separate from Bass Station Rack SysEx.

## Bytes 0..25

The desktop loader feeds ordinary bytes to the host-parameter layer as
`raw / 255`.  Three fields have explicit integer pre-transforms before that
normalization:

- byte 3, MIX: raw 0..84 clamps to code 0; raw 85..255 maps to
  `floor((raw-85)*255/170)`.
- byte 6, OSC1 PULSE WIDTH: raw 0 -> 255; otherwise
  `floor(raw*245/255)+10`.
- byte 24, OSC2 PULSE WIDTH: same transform as OSC1.

OSC2 SEMI (byte 2) is an ordinary normalized host value.  Parameter 8 in the
installer XML is integer 0..12, so FM1 maps it to the public 0..+12 semitone
control rather than the older mistaken signed interpretation.

## Bytes 26..31

- byte 26: low nibble = Pitch Modwheel 0..15; high nibble = Filter Modwheel
  0..15.
- byte 27: low 3 bits = PMod Breath 0..7; bits 4..6 = PMod Aftertouch 0..7.
  Bit 3 of either nibble is ignored.
- byte 28: low 3 bits = FMod Breath 0..7; bits 4..6 = FMod Aftertouch 0..7.
  Bit 3 of either nibble is ignored.
- byte 29: low nibble = Env Bias Breath 0..15; high nibble = Env Bias
  Aftertouch 0..15.
- byte 30: low nibble = Osc Bend Depth; public range is 0..12.
- byte 31: compact LFO-sync code.  Recognized raw values are
  `0,2,3,4,6,8,9,12,16,18,24,32,36,48,72,96`, mapping to public indices
  0..15.  Other values, including raw 64, decode OFF.  The v2.2 compact loader
  does not produce public enum 16.

## Byte 32 switch packing

- bits 7: OSC1 waveform, inverted (`1=Saw`, `0=Square` on the compact wire).
- bits 5..6: OSC2 octave/range, direct 0..3.
- bit 4: OSC2 waveform, inverted.
- bit 2: filter slope, inverted (`0=24dB`, `1=12dB` on the compact wire).
- bits 0..1: LFO waveform: 0=Random, 1=Triangle, 2=Sawtooth.

## Byte 33 switch packing

- bit 6: oscillator sync.
- bits 4..5: software-native trigger encoding: 0=MULTI, 1=SINGLE,
  2/3=AUTO.
- bits 2..3: OSC1 PWM source: 0=LFO, 1=Manual, 2=Env2; value 3 resolves to
  Manual in the original loader.
- bits 0..1: OSC2 PWM source with the same mapping.

The trigger coding above is **not** the Rack SysEx coding.  The Rack bridge
keeps its historical 1=MULTI, 2=SINGLE, 3=AUTO mapping and its signed
Breath/Aftertouch modulation depths.
