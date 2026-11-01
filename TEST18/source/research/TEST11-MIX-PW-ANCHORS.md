# TEST11 MIX / pulse-width native-state anchors

TEST11 does **not** change the oscillator MIX or pulse-width transfer curves.
Instead it records a second independent official Novation Automap screenshot so
future A/B work has more than one known factory anchor.

## Factory 00 — Moog Bass

Recovered native record bytes include:

- OSC1 PW raw `0x03`; official Automap display: `-58`
- OSC2 PW raw `0x05`; official Automap display: `-57`
- MIX raw `0xAF`; official Automap display: `3`
- LFO speed raw `0x9E`; displayed value `79`
- LFO delay raw `0xB6`; displayed value `91`

The doubled LFO values are consistent with the known compact state packing, but
the MIX/PW values clearly are not a simple direct signed display mapping.

## Factory 12 — TB303 Eow Bass (preset 13/100)

Recovered native record bytes include:

- OSC1 PW raw `0x03`; official Automap display: `-58`
- MIX raw `0x4D`; official Automap display: `-64`
- LFO speed raw `0x9C`; displayed value `78`
- LFO delay raw `0x80`; displayed value `64`
- filter frequency raw `0x58`; displayed value `44`
- resonance raw `0xFF`; displayed value `127`

These anchors independently validate the recovered program and demonstrate that
MIX in particular cannot be reconstructed by simply reinterpreting its native
byte as the panel's signed value. Pulse-width also requires the original desktop
plug-in transfer/taper (or enough additional measured points) before changing the
FM1 DSP.

## Decision for TEST11

Keep every recovered factory byte unchanged and retain the existing normalized
FM1 control path. Do not invent a polynomial or piecewise curve from two points.
The next safe step is a direct desktop plug-in sweep/A-B capture or more official
factory display anchors.
