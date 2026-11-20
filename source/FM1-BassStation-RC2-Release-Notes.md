# FM1 Bass Station RC2

RC2 is firmware version 1.0.1.

- Home has a dedicated header area above Cutoff and Resonance and a larger preset-name panel. The help text below the preset name has been removed.
- Press SEL to open Presets. Turning PRESET or SELECT loads the corresponding patch immediately. ALGO switches between Factory and User and loads the first patch in that bank.
- The SEQ Generate and Mutate screens fill the display with four clearly labelled controls. The Piano Roll, Gate, Octave, Accent and Slide editors pair larger 16-step grids with live knob values, bar and step counts, and a playback marker.
- SELECT and ALGO control Cutoff and Resonance outside the preset browser. PRESET still selects a step or Generate panel on SEQ.

Use `FM1-BassStation-RC2.fwsc` for normal updates that preserve the device's USER bank. The separate full-flash M-Flasher image is for clean installations or recovery. It writes the full flash, including USER data and settings.

Both packages were built from the included source. The README shows captures from the firmware's simulator using the same display drawing code as the FM-1. The build and simulator tests were run without a physical unit.
