#!/usr/bin/env python3
"""Control a DSD TECH SH-UR01A USB relay over its serial port."""

import argparse
import glob
import os
import select
import sys
import termios
import time


DEFAULT_DEVICE = "/dev/cu.usbserial-110"
COMMANDS = {
    "test": (b"AT", b"OK"),
    "close": (b"AT+CH1=1", b"OK+CH1=1"),
    "open": (b"AT+CH1=0", b"OK+CH1=0"),
}


def find_device(requested):
    if requested:
        return requested
    if os.path.exists(DEFAULT_DEVICE):
        return DEFAULT_DEVICE

    # The SH-UR01A uses a CP2102N. On macOS it normally has a numbered
    # cu.usbserial device; avoid the Pioneer console's known "0001" port.
    candidates = [
        path for path in sorted(glob.glob("/dev/cu.usbserial-*"))
        if not path.endswith("-0001")
    ]
    if len(candidates) == 1:
        return candidates[0]
    if not candidates:
        raise RuntimeError("SH-UR01A serial device not found")
    raise RuntimeError(
        "multiple possible relay devices found; use --device: "
        + ", ".join(candidates)
    )


def configure_9600_8n1(fd):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = termios.B9600
    attrs[5] = termios.B9600
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def send_command(device, action, timeout):
    command, expected = COMMANDS[action]
    fd = os.open(device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure_9600_8n1(fd)
        os.write(fd, command)
        deadline = time.monotonic() + timeout
        response = bytearray()
        while time.monotonic() < deadline:
            readable, _, _ = select.select([fd], [], [], deadline - time.monotonic())
            if not readable:
                break
            chunk = os.read(fd, 256)
            if chunk:
                response.extend(chunk)
                if expected in response:
                    return response.decode("ascii", errors="replace").strip()
        text = response.decode("ascii", errors="replace").strip()
        raise RuntimeError(
            f"unexpected response from {device}: {text!r}; expected {expected.decode()!r}"
        )
    finally:
        os.close(fd)


def main():
    parser = argparse.ArgumentParser(
        description="Operate the Pioneer's power button through an SH-UR01A relay."
    )
    parser.add_argument("action", choices=("test", "on", "off", "restart"))
    parser.add_argument(
        "--device",
        help=f"serial device (default: {DEFAULT_DEVICE}, with safe autodetection)",
    )
    parser.add_argument(
        "--duration", type=float,
        help="override button hold: on defaults to 0.75s, off/restart to 7s",
    )
    parser.add_argument(
        "--timeout", type=float, default=2.0,
        help="seconds to wait for each relay response (default: 2)",
    )
    args = parser.parse_args()

    if args.duration is not None and args.duration <= 0:
        parser.error("--duration must be positive")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    try:
        device = find_device(args.device)
        if args.action == "test":
            response = send_command(device, args.action, args.timeout)
        else:
            duration = args.duration
            if duration is None:
                duration = {
                    "on": 0.75,
                    "off": 7.0,
                    "restart": 7.0,
                }[args.action]
            print(f"Holding Pioneer power button for {duration:g} seconds via {device}")
            send_command(device, "close", args.timeout)
            try:
                time.sleep(duration)
            finally:
                # Always release the button, including after Ctrl-C.
                response = send_command(device, "open", args.timeout)
        print(f"Relay {args.action}: {response}")
    except (OSError, RuntimeError) as error:
        print(f"sh_ur01a.py: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
