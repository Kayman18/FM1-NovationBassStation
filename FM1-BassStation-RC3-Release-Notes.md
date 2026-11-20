# FM1 Bass Station RC3

Firmware version 1.0.2 uses the updater ID `FM-1_20010002`.

- INIT, Factory and USER presets start at 50% Cutoff when recalled. The midpoint of the existing logarithmic cutoff range is about 224 Hz. The rule applies to the live sound and leaves the preset records intact.
- On the first boot into RC3, the active sound starts at 50% Cutoff. Use SAVE in RC3 to resume your current live settings on subsequent boots.
- Turn SELECT or ALGO to see both Cutoff and Resonance below the current page title. The overlay clears 1.2 seconds after the last turn. On Filter, the existing controls update directly. On Presets, SELECT browses patches and ALGO switches banks.
- The Home and Filter pages show Cutoff as a percentage and in hertz.

Use the FWSC for normal updates that preserve the USER bank. The clean-install M-Flasher writes the full flash, including USER data and settings.
