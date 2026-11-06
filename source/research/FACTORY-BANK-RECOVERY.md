# TEST14 factory-bank recovery and native-state notes

## Result

TEST14 contains all **100 original Bass Station software plug-in factory program
records and names**. The firmware no longer uses diagnostic stand-ins for the
factory bank.

Source state:

- file: `research/BassStationStereo-Factory.pchk`
- length: 5,517 bytes
- SHA-256: `82b6c04d602662d2d3745ec7f3208a90cf03f6db7df481b63dbc998afecd13df`
- state magic at byte 4: `tffp`

## Recovered layout

The state has a small wrapper followed by a complete plug-in bank:

- bytes 0..3: wrapper/version
- bytes 4..12: bank header beginning `tffp`
- bytes 13..3412: **100 x 34-byte compact program records**
- bytes 3413..5512: **100 x 21-byte fixed program names**
- byte 5513: selected program
- bytes 5514..5516: trailer

`research/extract_plugin_factory_bank.py` validates this layout and regenerates
`app/factory_bank.h` deterministically.

## Cross-validation

The recovery is not based on names alone:

1. Novation's Automap documentation shows the original plug-in on
   `Preset 1/100: Moog Bass` together with displayed values. Program 00 from the
   recovered state matches those values/structure, including wheel pitch, bend,
   oscillator/filter/envelope settings and packed switches.
2. A second independent Novation Automap screenshot shows factory preset
   `13/100: TB303 Eow Bass`. Its LFO/filter values match recovered program 12,
   and its MIX/PW display supplies another native-byte anchor.
3. The historical `cha_bsr` Bass Station Rack editor source (2000) documents the
   same compact field order used by the 34-byte state and the corresponding
   76-byte nibble-expanded Rack SysEx frame.
4. Public BassStationStereo NKS metadata in `jhorology/nks-presets-collection`
   independently labels the matching names as `BassStation Factory` programs.
5. All 100 records decode inside the documented software ranges and pass the
   sanitizer stress test.

## Important compatibility distinction

The plug-in and Rack are related but not identical parameter surfaces. TEST14
keeps plug-in-native values internally and translates at the Rack bridge. In
particular the software manual/front panel and the v2.2 XML specify OSC2 SEMI at 0..+12 semitones; DETUNE reaches +/-96 cents.
The Rack also uses 0..+12 semitones but has the narrower roughly +/-50-cent detune range.

## Native-state decoder status

TEST14 recovered the remaining non-linear and packed state transformations directly from the original v2.2 plug-in binaries. MIX and both pulse-width fields now use the desktop DLL's exact integer transforms; OSC2 SEMI is restored as 0..+12; Breath/Aftertouch modulation uses the software format's unsigned 3-bit packing; pitch-bend masks its low nibble; LFO-sync uses the DLL lookup table; and the software trigger switch is no longer confused with the Rack trigger coding.

See `research/NATIVE-STATE-DECODER.md` for the byte-by-byte map.  The remaining fidelity boundary is **DSP/audio transfer**, not compact-state decoding: filter/oscillator response and control-to-audio tapers still require listening/A-B validation against the desktop plug-in if bit-for-bit proprietary behavior is desired.
