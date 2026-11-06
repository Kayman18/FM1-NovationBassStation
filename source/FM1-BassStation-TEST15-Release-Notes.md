# FM1 Bass Station TEST15 — preset browser + full-screen status fix

## Hardware report addressed
TEST14 changed presets audibly, but the preset/bank line was not visible on the FM1. USER slots also appeared to be copies of one sound because empty USER slots intentionally load INIT and there was no visible EMPTY label.

## Root cause
The app uses a 240x144-pixel SRAM canvas (69,120 bytes). TEST14 drew its help/status rows at y=166, 181, 196 and 211, so the renderer clipped them below row 143. TEST15 does not enlarge the framebuffer. It renders the 240x240 LCD as a 144-pixel main band followed by a 96-pixel footer band, reusing the same SRAM buffer.

## Preset browser
- Press **SEL** from a normal page to open **PRESET BROWSER**.
- The page shows the bank, slot number and preset name prominently.
- **SELECT** or **PRESET** moves the browser highlight without changing the currently sounding patch.
- Press **SEL** again to load the highlighted patch.
- **REC** switches the browser view between FACTORY/INIT and USER/COMMUNITY without changing the sound until SEL loads a patch.
- **HOME** exits to OSCILLATOR 1.
- Empty USER slots explicitly show **EMPTY** and **EMPTY USER SLOT = INIT SOUND**.
- Factory INIT remains separate from factory programs 1..100; USER is 1..128.

## Normal-page footer
The lower 96 screen rows now actually render. They show the control shortcuts and the current preset/bank line, so normal parameter pages also identify the loaded patch.

## Compatibility
- No DSP parameter structure change.
- No USER-bank flash format change.
- Existing TEST14 USER slots remain compatible.
- Existing normal page numbers 0..12 are unchanged; PRESET BROWSER is appended as page 13.

## Validation
- Host UI/MIDI smoke: passed, including preset-browser highlight-vs-load behavior, browser bank switching, factory traversal, USER persistence through slot 128, and MIDI bank/program changes.
- DSP ASan/UBSan/float-cast-overflow: passed.
- Factory 100-program decode sanitizer: passed.
- Rack SysEx sanitizer/round-trip: passed.
- Bank preparation/import regression: passed.

## Hardware test focus
1. Press SEL and confirm the PRESET BROWSER is clearly visible.
2. Browse to factory 11 `TB303 Autoglide Bas`, 13 `TB303 Eow Bass`, 17 `Trance 1`, and 18 `Trance 2`; press SEL to load each.
3. Press REC inside the browser and confirm empty USER slots visibly say EMPTY.
4. Exit with HOME and confirm the normal page footer now shows the loaded preset name/number.
