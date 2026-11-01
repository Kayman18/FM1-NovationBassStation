# FM1 Bass Station TEST15

TEST15 retains the complete recovered 100-program factory bank, 128-slot persistent USER / COMMUNITY bank, corrected 20-second envelopes, MIDI-clock handling, exact-time portamento, TEST12 bipolar endpoint corrections, and TEST13's exact v2.2 MIX/PULSE-WIDTH state transforms. TEST15 retains TEST14's exact native-state decoder fixes (OSC2 SEMI 0..+12, trigger/LFO-sync/controller packing) and adds the FM1 preset-browser/UI repair. SEL opens a dedicated preset browser with large bank/slot/name display; SELECT or PRESET browses without loading; SEL loads; REC switches the browser bank; HOME exits. Empty USER slots are explicitly labelled EMPTY. TEST15 also fixes the previously invisible lower-screen status/help area by rendering the 240-pixel display in two bands using the existing 240x144 SRAM canvas.

## Build target firmware

    bash build.sh

The first target build expands the bundled JieLi compiler archive under `build-toolchain/`. Outputs are written to `release/`.

## Host simulator

    bash build.sh --host
    app/build/host/bassstation_host tests/ui_smoke.script tests/ui_out

## Focused DSP / factory / SysEx tests

    cc -std=c11 -O1 -g -DBASSSTATION_TEST \
      -fsanitize=address,undefined,float-cast-overflow \
      -fno-omit-frame-pointer -Ibase/include -Iengine \
      engine/bassstation.c tests/bassstation_engine_test.c -lm \
      -o tests/bassstation_engine_test_asan
    ASAN_OPTIONS=detect_leaks=0 tests/bassstation_engine_test_asan

    cc -std=c11 -O1 -g -DBASSSTATION_TEST \
      -fsanitize=address,undefined,float-cast-overflow \
      -fno-omit-frame-pointer -Ibase/include -Iengine \
      engine/bassstation.c engine/bassstation_sysex.c \
      tests/bassstation_sysex_test.c -lm \
      -o tests/bassstation_sysex_test_asan
    ASAN_OPTIONS=detect_leaks=0 tests/bassstation_sysex_test_asan

    cc -std=c11 -O1 -g -DBASSSTATION_TEST \
      -fsanitize=address,undefined,float-cast-overflow \
      -fno-omit-frame-pointer -Ibase/include -Iengine \
      engine/bassstation.c engine/bassstation_factory.c \
      tests/bassstation_factory_test.c -lm \
      -o tests/bassstation_factory_test_asan
    ASAN_OPTIONS=detect_leaks=0 tests/bassstation_factory_test_asan

    python3 tests/test_prepare_bank.py

## Banks

- BUILT-IN contains `INIT` plus the recovered 100-program original Bass Station plug-in factory bank.
- `SEL` opens the preset browser. In the browser, SELECT or PRESET moves the highlight, SEL loads, REC changes bank, and HOME exits.
- Outside the browser, `REC` toggles BUILT-IN / USER.
- USER contains 128 persistent native FM1 Bass Station slots for user and community patches.
- `SAVE` stores the selected USER slot.
- CC0 `0` selects BUILT-IN; non-zero selects USER.
- In BUILT-IN, Program Change 0..99 loads factory programs 00..99; local PRESET also includes INIT.

See `research/USER-BANK-IMPORT.md` and `tools/prepare_bassstation_bank.py` for importing compatible old Rack/MIDI banks and classic plug-in FXB/FXP/tffp states; collections over 128 patches are split into multiple load sets (ZIP containers are supported directly).

See `FM1-BassStation-TEST15-Release-Notes.md`, `research/PARAMETER-DOMAINS.md`, and
`research/FACTORY-BANK-RECOVERY.md`, and `research/NATIVE-STATE-DECODER.md` for the full recovery/validation notes.
