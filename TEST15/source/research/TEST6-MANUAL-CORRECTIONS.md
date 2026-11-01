# TEST6 plug-in manual corrections

These are implementation notes for changes made after TEST5.

Source: Novation Bass Station Analogue Modelling Synthesizer Plug-In User Manual.

- Envelope section (manual p.22): attack, decay and release extend exponentially to 20 seconds in the software plug-in.
- LFO section (manual p.25): the SAW waveform is described as a rising level, so the software LFO saw is rising rather than falling.
- Controller section (manual p.33): Modwheel PITCH controls LFO modulation/vibrato depth; Aftertouch PITCH and Breath PITCH are described the same way. They are not static pitch offsets.
- Recovered Automap mapping for factory program 00 `Moog Bass`: Modwheel PITCH is 5. TEST5 incorrectly loaded 4.

The Bass Station Rack SysEx conversion retains its hardware-specific ranges and is intentionally not used as the software-panel conversion table.
