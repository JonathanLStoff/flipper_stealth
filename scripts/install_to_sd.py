#!/usr/bin/env python3
"""Installs the Stealth FAP onto a Flipper Zero.

Recommended (reliable, no extra dependencies): pull the SD card and use
--sd-path pointing at wherever your OS mounted it (a card reader, or
qFlipper's built-in SD card file browser mounted as a folder).

    python scripts/install_to_sd.py --sd-path E:\\

Experimental fallback: push the file over USB while the Flipper is
plugged in, using the `pyflipper` package's CLI-over-serial storage
support. This talks to whatever's already flashed - it does NOT
build/flash the app itself, run `make build` first.

    python scripts/install_to_sd.py --serial COM5
    python scripts/install_to_sd.py --serial auto   # try to find it

If neither is given, the script tries to auto-detect a Flipper's serial
port and falls back to telling you to use --sd-path.
"""
import argparse
import shutil
import sys
from pathlib import Path

APPID = "stealth"
# `ufbt build` doesn't drop a .fap in the project dir - it builds into its
# own per-user cache, shared across every app you've built with ufbt.
FAP_SRC = Path.home() / ".ufbt" / "build" / f"{APPID}.fap"

# Must match fap_category in application.fam - that's the apps/ subfolder the
# Flipper's launcher lists the app under.
FAP_DEST_REL = Path("apps") / "Games" / f"{APPID}.fap"


def check_sources():
    if not FAP_SRC.exists():
        print(f"error: {FAP_SRC} not found (run `make build` first)", file=sys.stderr)
        sys.exit(1)


def install_via_drive(sd_path: Path):
    if not sd_path.exists():
        print(f"error: {sd_path} does not exist - is the SD card mounted there?", file=sys.stderr)
        sys.exit(1)

    fap_dest = sd_path / FAP_DEST_REL
    fap_dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(FAP_SRC, fap_dest)
    print(f"Copied {FAP_SRC.name} -> {fap_dest}")
    print("Done. Eject/reinsert the SD card in the Flipper if it was hot-swapped.")


def find_flipper_port():
    try:
        from serial.tools import list_ports
    except ImportError:
        return None
    for port in list_ports.comports():
        desc = f"{port.description} {port.manufacturer or ''} {port.product or ''}".lower()
        if "flipper" in desc:
            return port.device
    return None


def install_via_serial(port: str):
    try:
        from pyflipper.pyflipper import PyFlipper
    except ImportError:
        print(
            "error: `pyflipper` isn't installed (pip install pyflipper), "
            "or use --sd-path instead - it's the more reliable option.",
            file=sys.stderr,
        )
        sys.exit(1)

    if port == "auto":
        found = find_flipper_port()
        if not found:
            print(
                "error: couldn't auto-detect a Flipper serial port. "
                "Pass --serial COM5 (or /dev/ttyACM0) explicitly, or use --sd-path.",
                file=sys.stderr,
            )
            sys.exit(1)
        port = found
        print(f"Found Flipper on {port}")

    print(f"Connecting to {port}...")
    flipper = PyFlipper(com=port)

    fap_dest = "/ext/" + str(FAP_DEST_REL).replace("\\", "/")

    print("This is experimental - if it fails, use --sd-path instead.")
    try:
        flipper.storage.mkdir(str(Path(fap_dest).parent).replace("\\", "/"))
        flipper.storage.send_file(str(FAP_SRC), fap_dest)
    except Exception as exc:  # pyflipper's surface differs across versions
        print(f"error: serial install failed ({exc}). Use --sd-path instead.", file=sys.stderr)
        sys.exit(1)

    print(f"Sent {FAP_SRC.name} -> {fap_dest}")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--sd-path", help="Path to the SD card mounted as a drive (recommended).")
    parser.add_argument(
        "--serial", help="Serial port (e.g. COM5), or 'auto' to detect it. Experimental."
    )
    args = parser.parse_args()

    check_sources()

    if args.sd_path:
        install_via_drive(Path(args.sd_path))
    elif args.serial:
        install_via_serial(args.serial)
    else:
        auto_port = find_flipper_port()
        if auto_port:
            print(f"No --sd-path given; found a Flipper on {auto_port}, trying serial install.")
            install_via_serial(auto_port)
        else:
            print(
                "No --sd-path or --serial given, and no Flipper found on serial.\n"
                "Recommended: mount the SD card and run:\n"
                "  python scripts/install_to_sd.py --sd-path <drive>",
                file=sys.stderr,
            )
            sys.exit(1)


if __name__ == "__main__":
    main()
