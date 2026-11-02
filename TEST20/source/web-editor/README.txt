FM1 Bass Station TEST20 USB-C Web Editor
========================================

This is a static local webpage. It does not require firmware networking and it
never uploads the synth state anywhere. The browser talks directly to the FM1
through the existing class-compliant USB-MIDI connection over USB-C using Web
MIDI SysEx.

Linux/macOS:
  ./start-editor.sh

Windows:
  double-click start-editor.bat

Then click CONNECT USB-C. Chrome or Edge are recommended because the page needs
Web MIDI with SysEx permission.

The editor controls all 48 Bass Station parameters exposed by the TEST20 engine,
Factory/User preset loading, JSON state save/load, and the complete 64-step 303
sequencer editor including Generate, variable-strength mutation, piano roll,
Gate, Up/Down octave, Accent and Slide editing.
