#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export AC79_SDK="${AC79_SDK:-$PWD/build-deps/sdk}"
export X0X_FONTSET=barlow

# Use an externally installed Jieli toolchain when supplied. Otherwise use the
# project-local extracted copy, unpacking the known archive on first build.
if [[ -n "${JIELI_TOOLCHAIN:-}" && -x "$JIELI_TOOLCHAIN/pi32v2/bin/clang" ]]; then
    t="$JIELI_TOOLCHAIN"
else
    t="$PWD/build-toolchain/jieli-linux-toolchains-20260730.1"
    if [[ ! -x "$t/pi32v2/bin/clang" ]]; then
        archive="$PWD/toolchain/jieli-linux-toolchains-20260730.1.tar.xz"
        if [[ ! -f "$archive" ]]; then
            echo "Missing Jieli toolchain." >&2
            echo "Set JIELI_TOOLCHAIN to an extracted jieli-linux-toolchains-20260730.1 directory" >&2
            echo "or place jieli-linux-toolchains-20260730.1.tar.xz in ./toolchain/." >&2
            exit 2
        fi
        mkdir -p build-toolchain
        tar --no-same-owner -xJf "$archive" -C build-toolchain
    fi
fi
export JIELI_TOOLCHAIN="$t"

release_ver="0.4.1"
python3 base/tools/build.py app --release "$release_ver" --display-version "TEST21" "$@"
if [[ " $* " == *" --host "* || " $* " == *" --web "* ]]; then exit 0; fi
mkdir -p release
fw="app/build/bassstation-${release_ver}.fwsc"
if [[ ! -f "$fw" ]]; then echo "Expected package not found: $fw" >&2; exit 3; fi
cp "$fw" release/FM1-BassStation-TEST21.fwsc

# The FWSC loader writes only the application area and is the normal update
# path when USER-bank contents must be preserved.  A generic full 1 MiB
# M-Flasher necessarily contains the pinned reference image's data sectors,
# not this individual FM1's current USER bank, so build it only when explicitly
# requested for clean-install/recovery use.
if [[ "${BUILD_CLEAN_MFLASHER:-0}" == "1" ]]; then
    python3 tools/make_mflasher.py \
        reference/FM1-PolyWavetable-TEST81-M-Flasher.bin \
        release/FM1-BassStation-TEST21-CLEAN-INSTALL-M-Flasher.bin \
        --app app/build/bassstation.bin
else
    echo "FWSC built. Generic M-Flasher skipped to protect per-device USER data."
    echo "Set BUILD_CLEAN_MFLASHER=1 only for a clean-install/recovery image."
fi
