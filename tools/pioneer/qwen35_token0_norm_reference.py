#!/usr/bin/env python3
"""Independent offline reference for token 0's first RMS-normalized rows.

Arguments are the verified 32 MiB GGUF prefix and the 10,240-byte layer-zero
attention-norm weight slice at GGUF offset 538348704. This diagnostic does not
load a driver or execute inference in Work mode. The computation follows the
GGUF Q6_K layout directly and uses host double arithmetic to check the sampled
values, not the device implementation or its reduction helper. An optional
token ID is accepted only when its entire embedding is in the pinned prefix.
"""
import hashlib
import math
import struct
import sys
from pathlib import Path

PREFIX_SHA256 = "4a8c4eefb927617012ea0da30a8fe76f2c8f4857fe573d82c0a1e6e9562f836d"
NORM_SHA256 = "fd26aeb9265ba4588eca258e4c7fb27d2b82198eef72bbca4d8d46136b90c104"
EMBEDDING_OFFSET = 10978464


def checked(path, length, digest):
    data = Path(path).read_bytes()
    if len(data) != length or hashlib.sha256(data).hexdigest() != digest:
        raise SystemExit(f"input does not match the verified model slice: {path}")
    return data


def normalized(prefix_path, norm_path, token=0):
    prefix = checked(prefix_path, 32 * 1024 * 1024, PREFIX_SHA256)
    norm = checked(norm_path, 2560 * 4, NORM_SHA256)
    token_start = EMBEDDING_OFFSET + token * 2100
    if token < 0 or token_start + 2100 > len(prefix):
        raise ValueError("token embedding lies outside the verified prefix")
    embedding = []
    for block in range(10):
        base = token_start + block * 210
        data = prefix[base : base + 210]
        multiplier = struct.unpack_from("<e", data, 208)[0]
        values = [0.0] * 256
        for half in (0, 128):
            low_base, high_base, scale_base = half // 2, 128 + half // 4, 192 + half // 16
            for lane in range(32):
                for quarter in range(4):
                    low_index = low_base + lane + (32 if quarter & 1 else 0)
                    low = (data[low_index] >> (4 if quarter >= 2 else 0)) & 15
                    high = ((data[high_base + lane] >> (2 * quarter)) & 3) << 4
                    scale = struct.unpack_from("<b", data, scale_base + lane // 16 + 2 * quarter)[0]
                    values[half + lane + 32 * quarter] = multiplier * scale * ((low | high) - 32)
        embedding.extend(values)
    weights = struct.unpack("<2560f", norm)
    inverse_rms = 1.0 / math.sqrt(sum(value * value for value in embedding) / 2560 + 1e-6)
    return [value * inverse_rms * weight for value, weight in zip(embedding, weights)]


def main():
    if len(sys.argv) not in (3, 4):
        raise SystemExit("usage: qwen35_token0_norm_reference.py GGUF_PREFIX NORM_SLICE [TOKEN_ID]")
    token = int(sys.argv[3]) if len(sys.argv) == 4 else 0
    values = normalized(sys.argv[1], sys.argv[2], token)
    for row in (16, 31):
        value = values[row]
        bits = struct.unpack("<I", struct.pack("<f", value))[0]
        print(f"token={token} layer=0 row={row} bits=0x{bits:08x} value={value:.9g}")


if __name__ == "__main__":
    main()
