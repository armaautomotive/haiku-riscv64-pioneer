#!/usr/bin/env python3
"""Check the probe's raw RISC-V Image header; this is not a behavior proof."""
import argparse
import hashlib
import struct


def verify(data, *, maximum_size=131072):
    if len(data) < 64:
        raise ValueError("truncated Image header")
    (code, nop, offset, size, flags, version, reserved1, reserved2,
     magic, magic2, pe_offset) = struct.unpack_from("<IIQQQIIQ8sII", data)
    if code != 0x0400006f or nop != 0x13:
        raise ValueError("expected uncompressed jump to entry at byte 64")
    if offset != 0x200000 or size != len(data) or size % 4096 != 0:
        raise ValueError("incorrect load offset, image size or page alignment")
    if size < 24576 or size > maximum_size:
        raise ValueError("size outside probe bounds")
    if flags != 0 or version != 2 or reserved1 != 0 or reserved2 != 0:
        raise ValueError("unexpected flags, version or reserved fields")
    if magic != b"RISCV\0\0\0" or magic2 != 0x05435352 or pe_offset != 0:
        raise ValueError("invalid RISC-V magic or unexpected EFI header")
    if not any(data[64:96]):
        raise ValueError("entry code is missing")
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-size", type=lambda value: int(value, 0), default=131072,
                        help="explicit header-only size cap for larger loader smoke images")
    parser.add_argument("image")
    args = parser.parse_args()
    with open(args.image, "rb") as stream:
        if args.max_size < 24576 or args.max_size > 16777216:
            parser.error("--max-size must be between 24576 and 16777216")
        data = stream.read(args.max_size + 1)
    digest = verify(data, maximum_size=args.max_size)
    print(f"PASS: raw RISC-V Image, {len(data)} bytes, SHA-256 {digest}")


if __name__ == "__main__":
    main()
