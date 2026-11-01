#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export AC79_SDK="${AC79_SDK:-$PWD/build-deps/sdk}"
export X0X_FONTSET=barlow
t="$PWD/build-toolchain/jieli-linux-toolchains-20260730.1"
if [[ ! -x "$t/pi32v2/bin/clang" ]]; then
 mkdir -p build-toolchain
 tar --no-same-owner -xJf toolchain/jieli-linux-toolchains-20260730.1.tar.xz -C build-toolchain
fi
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$t}"
python3 base/tools/build.py app --release 0.3.0 --display-version "TEST18" "$@"
if [[ " $* " == *" --host "* || " $* " == *" --web "* ]]; then exit 0; fi
mkdir -p release
fw=$(find app/build -maxdepth 1 -name '*.fwsc' -print -quit)
cp "$fw" release/FM1-BassStation-TEST18.fwsc
python3 tools/make_mflasher.py reference/FM1-PolyWavetable-TEST81-M-Flasher.bin release/FM1-BassStation-TEST18-M-Flasher.bin --app app/build/bassstation.bin
