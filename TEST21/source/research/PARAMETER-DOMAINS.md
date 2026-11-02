# Bass Station parameter domains (TEST16 notes)

The classic Bass Station software and Bass Station Rack expose several related but **not interchangeable** parameter encodings.  TEST12 keeps them separate instead of forcing every source through one generic 0..255 mapping.

## 1. Plug-in public host / MIDI controls

The Bass Station 2.1 host parameter map exposes 128-position controls.  Bipolar zero-centered parameters use host center `64/127` and display zero there.  For a conventional bipolar parameter, TEST12 therefore treats the public control as signed `-64..+63` and maps negative and positive sides independently:

- raw 0 -> -1.0
- raw 1 -> -63/64
- raw 64 -> 0.0
- raw 127 -> +1.0

This avoids TEST11's `value / 63` clamp, where raw 0 and raw 1 both became the same full-negative value.

The public MIX control is known to be `-64 = OSC1`, `0 = equal`, `+63 = OSC2`; its MIDI/host path already maps 0..127 across oscillator 1..2. Pulse Width uses the same displayed bipolar range contextually. In **MAN** mode the plug-in manual states that the centre (`0`) is the 50% square wave and moving either clockwise or anticlockwise narrows the pulse toward opposite polarities. In **LFO/ENV2** mode the centre is zero modulation depth and the two sides invert modulation direction. TEST15's DSP interpretation already follows this behavior; TEST16 does not change it. Its exact saved native `.pchk` byte transfer is a separate issue below.

## 2. Recovered native plug-in program records

The recovered factory bank stores 34-byte compact program records.  Several ordinary bipolar fields behave like a full signed 8-bit quantity around raw 128. TEST14 maps them as `-128..+127` with separate negative/positive divisors, so raw 0 and raw 1 remain distinct and raw 255 reaches the exact positive endpoint.

TEST14 recovers the exact MIX and PULSE WIDTH transforms from the original Novation Bass Station v2.2 plug-in DLL (both x86 and x64 builds contain the same integer sequence). They are not ordinary signed bytes:

- MIX: native values `0..84` clamp to public normalized code `0`; values `85..255` map as `floor((raw-85)*255/170)`.
- PULSE WIDTH: native `0` is a special public code `255`; values `1..255` map as `floor(raw*245/255)+10`.

These formulas reproduce the official Automap anchors exactly:

- Moog Bass native MIX `175` -> public code `135` -> display `+3`
- TB303 Eow Bass native MIX `77` -> public code `0` -> display `-64`
- Moog Bass native OSC1 PW `3` -> public code `12` -> display `-58`
- Moog Bass native OSC2 PW `5` -> public code `14` -> display `-57`

Only recovered software plug-in records use these transforms. Rack SysEx remains in its historical full-byte domain.

The same v2.2 audit also fixes the compact fields that are not ordinary bytes:

- OSC2 SEMI is public `0..+12`.
- software Breath/Aftertouch pitch/filter depths are unsigned 3-bit `0..7` fields; Rack depths remain signed.
- software trigger bits are `0=MULTI, 1=SINGLE, 2/3=AUTO`; Rack trigger coding is different.
- native LFO-sync recognizes raw `0,2,3,4,6,8,9,12,16,18,24,32,36,48,72,96` only.

The exact byte map is in `research/NATIVE-STATE-DECODER.md`.

## 3. Bass Station Rack SysEx

The historical Rack single-program SysEx stores the main parameters as full 8-bit values split into MIDI-safe nibbles.  Period editor source (`cha_bsr`, 2000) confirms that MIX/PWM and the other main fields are kept at full 8-bit precision in SysEx; only controller messages for parameters with MIDI CC support are reduced to 7-bit.

TEST14 uses the same `-128..+127` endpoint geometry for Rack bipolar bytes and now round-trips both exact endpoints:

- raw 0 -> exact negative endpoint
- raw 128 -> zero
- raw 255 -> exact positive endpoint

Rack MIX/PW remain full-byte Rack values and are **not** passed through the software plug-in `.pchk` calibration path.

## Sources used for this audit

- Novation Bass Station software manual / Automap documentation (original parameter ranges and controller numbers).
- `jhorology/nks-presets-collection`, BassStationStereo 2.1 direct parameter mapping (host normalized center values).
- Claude Heiland-Allen `cha_bsr` Bass Station Rack editor source, 2000, mirrored by AmigaTextFiles (full-byte Rack SysEx structure).

This separation remains important: the recovered compact plug-in state, Rack SysEx, and public host/MIDI controls are different domains and must not share one generic byte conversion.
