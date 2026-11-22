#!/usr/bin/env python3
"""Read two independent Q6_K weight samples; this does not run inference.

Input is the first 32 MiB of the verified 4B GGUF, not a model upload.  The
pinned prefix hash fixes the tensor-table offset and the two source samples.
The arithmetic follows llama.cpp's Q6_K layout directly, without using the
rewrite's tile packer or device embedding implementation.
"""
import hashlib
import struct
import sys
from pathlib import Path

PREFIX_BYTES = 32 * 1024 * 1024
PREFIX_SHA256 = "4a8c4eefb927617012ea0da30a8fe76f2c8f4857fe573d82c0a1e6e9562f836d"
TOKEN_EMBEDDING_OFFSET = 10978464


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: qwen35_token0_embedding_reference.py GGUF_PREFIX")
    prefix = Path(sys.argv[1]).read_bytes()
    if len(prefix) != PREFIX_BYTES or hashlib.sha256(prefix).hexdigest() != PREFIX_SHA256:
        raise SystemExit("GGUF prefix does not match the verified model")
    block = prefix[TOKEN_EMBEDDING_OFFSET : TOKEN_EMBEDDING_OFFSET + 210]
    if len(block) != 210:
        raise SystemExit("truncated Q6_K block")
    multiplier = struct.unpack_from("<e", block, 208)[0]
    for row in (16, 31):
        quant = ((block[row] & 15) | ((block[128 + row] & 3) << 4)) - 32
        scale = struct.unpack_from("<b", block, 192 + row // 16)[0]
        value = f32(f32(multiplier * scale) * quant)
        bits = struct.unpack("<I", struct.pack("<f", value))[0]
        print(f"token=0 row={row} bits=0x{bits:08x} value={value:.9g}")


if __name__ == "__main__":
    main()
