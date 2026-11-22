#!/usr/bin/env python3
"""Approximate independent first-panel reference, computed offline only.

The final block-0/panel-0 sampled by core0 is rows48..63 of layer0 QKV.
The three inputs are pinned GGUF slices: the prefix, norm weights, and the
28,160 raw Q5_K bytes at offset 538443424. Quantization uses ordinary host
floating-point arithmetic, so use these values as a diagnostic comparison,
not a proof of bit-identical device rounding or an inference fallback. An
optional token ID must have its full embedding in the hash-pinned prefix.
"""
import struct
import sys
from qwen35_token0_norm_reference import checked, normalized

ROWS_SHA256 = "fed184217496d56036dcdc689d339d7d023ee6b3857bf941a8681494769d3083"


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def main():
    if len(sys.argv) not in (4, 5):
        raise SystemExit("usage: qwen35_token0_projection_reference.py GGUF_PREFIX NORM_SLICE QKV_ROWS [TOKEN_ID]")
    token = int(sys.argv[4]) if len(sys.argv) == 5 else 0
    raw_rows = checked(sys.argv[3], 16 * 1760, ROWS_SHA256)
    activations = [f32(value) for value in normalized(sys.argv[1], sys.argv[2], token)[:256]]
    selected = max(activations, key=abs)
    inverse = f32(-127.0 / selected)
    scale = f32(1.0 / inverse)
    quant = [round(f32(value * inverse)) for value in activations]
    if any(value < -127 or value > 127 for value in quant):
        raise SystemExit("quantized activation is out of range")
    def encode(value):
        return 0x4000 | (0x8000 | -value if value < 0 else value)
    words = [encode(quant[i]) | (encode(quant[i + 1]) << 16) for i in range(0, 32, 2)]
    dots, coefficients, minima, accumulated = [], [], [], []
    input_sum = sum(quant[:16])
    for row in range(16):
        block = raw_rows[row * 1760 : row * 1760 + 176]
        d = struct.unpack_from("<e", block, 0)[0]
        coefficient = f32(d * (block[4] & 63))
        minimum = f32(-struct.unpack_from("<e", block, 2)[0] * (block[8] & 63))
        coefficients.append(coefficient)
        minima.append(minimum)
        values = [(block[48 + lane] & 15) | ((block[16 + lane] & 1) << 4)
                  for lane in range(16)]
        dot = sum(value * quant[lane] for lane, value in enumerate(values))
        dots.append(dot)
        product = f32(f32(coefficient * scale) * dot)
        correction = f32(f32(minimum * scale) * input_sum)
        accumulated.append(f32(product + correction))
    print(f"packed_activation first=0x{words[0]:08x} last=0x{words[-1]:08x}")
    print(f"activation_scale first=0x{bits(scale):08x}")
    print(f"matrix_dot first={dots[0]} last={dots[-1]} first_bits=0x{dots[0]&0xffffffff:08x} last_bits=0x{dots[-1]&0xffffffff:08x}")
    print(f"affine_coefficient first=0x{bits(coefficients[0]):08x} last=0x{bits(coefficients[-1]):08x}")
    print(f"affine_minimum first=0x{bits(minima[0]):08x} last=0x{bits(minima[-1]):08x}")
    print(f"first_panel_accumulator first=0x{bits(accumulated[0]):08x} last=0x{bits(accumulated[-1]):08x}")


if __name__ == "__main__":
    main()
