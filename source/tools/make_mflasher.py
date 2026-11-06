#!/usr/bin/env python3
"""Package the mono application into the user's existing full FM1 backup.

Only [0x4000,0x93000) is replaced. All boot, calibration, settings and data
bytes outside that interval are retained verbatim and verified. No device is
opened or flashed by this program.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "base/tools"))
import fm1pkg_make as package

START, END, SIZE = 0x4000, 0x93000, 0x100000
SUPPORTED_BASE_SHA256 = "ead915f71caff82d92e090da0b6912e910774fff950153fd4976f935937e6940"

def sha(data):
    return hashlib.sha256(data).hexdigest()

def read_entry(image, offset, encrypted=True):
    raw = bytearray(image[offset:offset + 32])
    if len(raw) != 32:
        raise ValueError("Truncated directory entry")
    if encrypted:
        package.enc(raw, 0, 32)
    if package.crc16(raw[2:]) != struct.unpack_from("<H", raw)[0]:
        raise ValueError(f"Directory CRC mismatch at {offset:#x}")
    return raw

def make(base, app):
    if len(base) != SIZE:
        raise ValueError("The input must be an existing full 1 MiB FM1 backup")
    if sha(base) != SUPPORTED_BASE_SHA256:
        raise ValueError("This test packager requires the bundled, SHA256-pinned TEST81 backup")
    if not app or len(app) > package.APP_SLOT or app[:4] != bytes.fromhex("04818000"):
        raise ValueError("Application entry stub or application size is invalid")
    read_entry(base, 0)
    spl = read_entry(base, 0x20)
    config = read_entry(base, 0x40)
    app_entry = read_entry(base, 0x60)
    if spl[16:26] != b"uboot.boot" or config[16:30] != b"isd_config.ini":
        raise ValueError("This is not the supported FM1 flash layout")
    spl_pos, spl_len = struct.unpack_from("<II", spl, 4)
    if spl_pos < 0xA0 or spl_pos + spl_len > START:
        raise ValueError("Unexpected preserved bootloader bounds")
    # This known backup retains an older encoded SPL. Its on-flash bytes do
    # not have the bundled SDK's plain BANK representation or raw-data CRC.
    # Pin the entire known backup and preserve its boot region verbatim;
    # never replace that bootloader or pretend its raw CRC is a plain CRC.
    pos, length = struct.unpack_from("<II", config, 4)
    if length < 32 or pos + length > START:
        raise ValueError("Invalid backup configuration location")
    config_bytes = base[pos:pos + length]
    if package.crc16(config_bytes) != struct.unpack_from("<H", config, 2)[0]:
        raise ValueError("Backup configuration CRC is invalid")
    if struct.unpack_from("<I", app_entry, 4)[0] != START:
        raise ValueError("Unexpected application offset")
    key = package.chipkey_decode(config_bytes[:32])
    generated = package.flash_image(app, key)
    image = bytearray(base)
    image[START:END] = generated[START:END]
    if len(image) != SIZE or image[:START] != base[:START] or image[END:] != base[END:]:
        raise ValueError("A byte outside the application region changed")
    # Decode the actual output with the key from its preserved configuration.
    decoded = bytearray(image[START:END])
    package.sfc(decoded, 0, len(decoded), 0, key)
    header = read_entry(decoded, 0, encrypted=False)
    block_size = struct.unpack_from("<I", header, 8)[0]
    if block_size > len(decoded) or package.crc16(decoded[32:block_size]) != struct.unpack_from("<H", header, 2)[0]:
        raise ValueError("Application directory content CRC mismatch")
    entry = read_entry(decoded, 32, encrypted=False)
    pos, length = struct.unpack_from("<II", entry, 4)
    if entry[16:23] != b"app.bin" or length != package.APP_SLOT:
        raise ValueError("Unexpected output application entry")
    payload = decoded[pos:pos + length]
    expected = app + b"\xff" * (package.APP_SLOT - len(app))
    if payload != expected or package.crc16(payload) != struct.unpack_from("<H", entry, 2)[0]:
        raise ValueError("Packaged application does not match the linked application")
    report = {
        "full_image_bytes": len(image), "application_bytes": len(app),
        "application_slot_bytes": package.APP_SLOT,
        "base_sha256": sha(base), "application_sha256": sha(app), "image_sha256": sha(image),
        "changed_region_start": START, "changed_region_end_exclusive": END,
        "boot_region_preserved": image[:START] == base[:START],
        "boot_validation": "exact bytes of the SHA256-pinned TEST81 backup",
        "all_bytes_after_application_preserved": image[END:] == base[END:],
        "panel_calibration_preserved": image[0xDC000:0xDE000] == base[0xDC000:0xDE000],
        "settings_preserved": image[0xFC000:] == base[0xFC000:],
        "directory_crcs_valid": True, "decrypted_application_exact": True,
        "flashed_or_executed_on_hardware": False
    }
    return bytes(image), report

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--app", type=Path, default=ROOT / "app/build/ms20xxl.bin")
    parser.add_argument("--sdk", type=Path, default=ROOT / "build-deps/sdk")
    args = parser.parse_args()
    package.SDK = args.sdk
    image, report = make(args.base.read_bytes(), args.app.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)
    args.output.with_suffix(".json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
