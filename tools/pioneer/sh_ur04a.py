#!/usr/bin/env python3
"""Control a DSD TECH SH-UR04A used with the Milk-V Pioneer."""

import argparse
import glob
import os
import select
import sys
import termios
import time


DEFAULT_DEVICE = "/dev/cu.usbserial-1"
CONSOLE_DEVICE = "/dev/cu.usbserial-0001"
POWER_CHANNEL = 1
SD_CHANNEL = 2


def find_device(requested):
    if requested:
        return requested
    if os.path.exists(DEFAULT_DEVICE):
        return DEFAULT_DEVICE

    candidates = [
        path for path in sorted(glob.glob("/dev/cu.usbserial-*"))
        if path != CONSOLE_DEVICE
    ]
    if len(candidates) == 1:
        return candidates[0]
    if not candidates:
        raise RuntimeError("SH-UR04A serial device not found")
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


class Relay:
    def __init__(self, device, timeout):
        self.device = device
        self.timeout = timeout
        self.fd = None

    def __enter__(self):
        self.fd = os.open(
            self.device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK
        )
        configure_9600_8n1(self.fd)
        # The SH-UR04A can lose the first byte if written immediately on open.
        time.sleep(0.2)
        termios.tcflush(self.fd, termios.TCIOFLUSH)
        return self

    def __exit__(self, _type, _value, _traceback):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None

    def command(self, command, expected):
        termios.tcflush(self.fd, termios.TCIFLUSH)
        os.write(self.fd, command.encode("ascii") + b"\r\n")
        deadline = time.monotonic() + self.timeout
        response = bytearray()
        if isinstance(expected, str):
            expected = (expected,)
        expected_bytes = tuple(value.encode("ascii") for value in expected)

        while time.monotonic() < deadline:
            remaining = max(0, deadline - time.monotonic())
            readable, _, _ = select.select([self.fd], [], [], remaining)
            if not readable:
                break
            chunk = os.read(self.fd, 256)
            if chunk:
                response.extend(chunk)
                if any(value in response for value in expected_bytes):
                    return response.decode("ascii", errors="replace").strip("\x00\r\n ")

        text = response.decode("ascii", errors="replace").strip("\x00\r\n ")
        raise RuntimeError(
            f"unexpected response from {self.device}: {text!r}; "
            f"expected one of {expected!r}"
        )

    def test(self):
        return self.command("AT", "OK")

    def set_channel(self, channel, closed):
        value = 1 if closed else 0
        command = f"AT+CH{channel}={value}"
        return self.command(command, f"OK+CH{channel}={value}")

    def channel_state(self, channel):
        response = self.command(
            f"AT+CH{channel}=?",
            (f"OK+CH{channel}=0", f"OK+CH{channel}=1"),
        )
        if response.endswith("=1"):
            return True
        if response.endswith("=0"):
            return False
        raise RuntimeError(f"invalid channel {channel} state response: {response!r}")


def pulse_power(relay, duration):
    relay.set_channel(POWER_CHANNEL, True)
    try:
        time.sleep(duration)
    finally:
        # Always release the power button, including after Ctrl-C.
        relay.set_channel(POWER_CHANNEL, False)


def main():
    parser = argparse.ArgumentParser(
        description="Control Pioneer power on relay 1 and SD attachment on relay 2."
    )
    parser.add_argument(
        "action",
        choices=(
            "test", "status", "on", "off", "restart",
            "sd-attach", "sd-detach", "sd-status",
        ),
    )
    parser.add_argument(
        "--device",
        help=f"relay serial device (default: {DEFAULT_DEVICE}, with autodetection)",
    )
    parser.add_argument(
        "--duration", type=float,
        help="override power-button hold duration",
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
    if args.duration is not None and args.action not in ("on", "off", "restart"):
        parser.error("--duration applies only to on, off, and restart")

    try:
        device = find_device(args.device)
        with Relay(device, args.timeout) as relay:
            if args.action == "test":
                print(f"SH-UR04A test: {relay.test()}")
            elif args.action in ("status", "sd-status"):
                if args.action == "status":
                    power = "closed" if relay.channel_state(POWER_CHANNEL) else "open"
                    print(f"Relay 1 (power button): {power}")
                attached = relay.channel_state(SD_CHANNEL)
                print(f"Relay 2 (SD): {'attached' if attached else 'detached'}")
            elif args.action == "sd-attach":
                relay.set_channel(SD_CHANNEL, True)
                print("Relay 2: SD attached")
            elif args.action == "sd-detach":
                relay.set_channel(SD_CHANNEL, False)
                print("Relay 2: SD detached")
            else:
                duration = args.duration
                if duration is None:
                    duration = {
                        "on": 0.75,
                        "off": 7.0,
                        "restart": 7.0,
                    }[args.action]
                print(f"Holding Pioneer power button for {duration:g} seconds")
                pulse_power(relay, duration)
                print(f"Pioneer power action complete: {args.action}")
    except (OSError, RuntimeError) as error:
        print(f"sh_ur04a.py: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
