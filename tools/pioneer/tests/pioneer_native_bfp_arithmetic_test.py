#!/usr/bin/env python3
"""Separate Blackhole BF16/FP32 characterization, NOT an acceptance oracle.

Integer implementation of arithmetic FACTS in the primary ISA reference:
https://github.com/tenstorrent/tt-isa-documentation/blob/4a73a7121d95f545b85efa34b243f0b8b316f967/WormholeB0/TensixTile/TensixCoprocessor/MatrixUnitArithmetic.md
This does not change the existing RV32 observer, diagnostic Expected(),
any comparator, hardware image, or permission to END a failed diagnostic.
Only finite normal/zero BF16 operands, Blackhole, FP32 accumulation and
stochastic rounding OFF are characterized here. No device is opened.
"""
import pathlib
import re
from fractions import Fraction

if not __debug__:
    raise SystemExit('Assertions required')


def power_of_two(exponent):
    return Fraction(2 ** exponent) if exponent >= 0 else Fraction(1, 2 ** -exponent)


def encode_normal(value, ties_away=False):
    """Encode an exact rational to normal binary32; zero/flush/overflow bounded."""
    if not value:
        return 0
    sign = int(value < 0)
    magnitude = abs(value)
    assert magnitude.denominator & (magnitude.denominator - 1) == 0
    top = magnitude.numerator.bit_length() - 1
    exponent = top - (magnitude.denominator.bit_length() - 1) + 127
    scaled = magnitude.numerator * power_of_two(23 - top)
    integral, remainder = divmod(scaled.numerator, scaled.denominator)
    comparison = remainder * 2 - scaled.denominator
    integral += int(comparison > 0 or (comparison == 0 and (ties_away or integral & 1)))
    if integral == 1 << 24:
        integral >>= 1
        exponent += 1
    if exponent <= 0:
        return 0
    if exponent >= 255:
        return (sign << 31) | 0x7f800000
    return (sign << 31) | (exponent << 23) | (integral & 0x7fffff)


def bf16_value(encoded):
    sign, exponent, fraction = encoded >> 15, (encoded >> 7) & 255, encoded & 127
    assert exponent != 255
    if not exponent:
        assert not fraction
        return Fraction(0)
    return (-1 if sign else 1) * Fraction(128 + fraction, 128) * power_of_two(exponent - 127)


def fixture_operand(bits, row, column):
    magnitude = (row * 37 + column * 19 + 11) & (7 if bits == 4 else 127)
    if not magnitude:
        return 0
    highest = magnitude.bit_length() - 1
    exponent = 126 + row % 3 - (bits - 2) + highest
    fraction = (magnitude << (7 - highest)) - 128
    sign = int((row * 3 + column * 5) & 3 == 2)
    return (sign << 15) | (exponent << 7) | fraction


def activation(column):
    return 0x3f00 + (column * 13) % 256 + (0x8000 if column & 3 == 2 else 0)


def positive_nearest(value, shift):
    """Magnitude rounding to nearest, ties away from zero."""
    assert value >= 0 and shift >= 0
    return (value + (1 << (shift - 1))) >> shift if shift else value


def signed_nearest_up(value, shift):
    """Signed rounding to nearest, ties towards positive infinity."""
    assert shift >= 0
    return (value + (1 << (shift - 1))) // (1 << shift) if shift else value


def lane(operands_a, operands_b, phase):
    # Decode only the real source fields; no C floating products/sums.
    pairs = []
    for a, b in zip(operands_a, operands_b):
        ae, be = (a >> 7) & 255, (b >> 7) & 255
        am, bm = (a & 127) << 3, (b & 127) << 3
        ma = ((am >> 1) & 31) if phase & 1 else (16 | (am >> 6))
        mb = ((bm & 15) << 3) if phase & 2 else (64 | (bm >> 4))
        pairs.append((ae + be if ae and be else 0, ma * mb if ae and be else 0, bool((a ^ b) & 0x8000)))
    largest = max(pair[0] for pair in pairs)
    total = 0
    for exponent, product, negative in pairs:
        distance = largest - exponent
        aligned = positive_nearest(product, distance) if distance < 16 else 0
        total += -aligned if negative else aligned
    rebased = (largest - 127 - (5 if phase & 1 else 0) - (7 if phase & 2 else 0)) % 1024
    return (rebased, total) if 0 < rebased < 512 else (0, 0)


def instruction(operands_a, operands_b, phase, previous):
    assert len(operands_a) == len(operands_b) == 16 and 0 <= phase < 4
    lanes = [lane(operands_a[begin:begin + 8], operands_b[begin:begin + 8], phase) for begin in (0, 8)]
    destination_exponent = (previous >> 23) & 255
    destination_magnitude = (0x800000 | (previous & 0x7fffff)) if destination_exponent else 0
    common = max(destination_exponent, *(item[0] for item in lanes))
    fixed = sum(signed_nearest_up(value * 8192, min(common - exponent, 31)) for exponent, value in lanes)
    destination = positive_nearest(destination_magnitude, min(common - destination_exponent, 31))
    fixed += -destination if previous >> 31 else destination
    return encode_normal(fixed * power_of_two(common - 127 - 23), ties_away=True)


def characterize(bits, row):
    a = [fixture_operand(bits, row, column) for column in range(32)]
    b = [activation(column) for column in range(32)]
    ideal = encode_normal(sum((bf16_value(x) * bf16_value(y) for x, y in zip(a, b)), Fraction(0)))
    predicted = 0
    for begin in (0, 16):
        for phase in ((0, 2) if bits == 4 else (0, 1, 2, 3)):
            predicted = instruction(a[begin:begin + 16], b[begin:begin + 16], phase, predicted)
    return ideal, predicted


assert positive_nearest(1, 1) == 1 and positive_nearest(3, 1) == 2
assert signed_nearest_up(-1, 1) == 0 and signed_nearest_up(-3, 1) == -1
assert encode_normal(Fraction(1) + Fraction(1, 1 << 24)) == 0x3f800000
assert encode_normal(Fraction(1) + Fraction(1, 1 << 24), ties_away=True) == 0x3f800001
assert encode_normal(-Fraction(1) - Fraction(1, 1 << 24), ties_away=True) == 0xbf800001
assert instruction([0] * 16, [0] * 16, 0, 0) == 0
assert instruction([0x3f80] * 16, [0x3f80] * 16, 0, 0) == 0x41800000

results = {}
for bits in (4, 8):
    results[bits] = [characterize(bits, row) for row in range(32)]
    for row, (ideal, predicted) in enumerate(results[bits]):
        print(f'CHARACTERIZATION bits{bits} row{row} ideal{ideal:08x} lane_model{predicted:08x} strict_equal{int(ideal == predicted)}')

# The recorded hardware run stopped at its FIRST failed word. Claim one
# BFP8 observation only; the other63 returned words were not recorded then.
here = pathlib.Path(__file__).resolve().parent
record = (here.parent / 'benchmarks/qwen38-native-bfp-physical-20261103/physical.log').read_text()
matches = re.findall(r'NATIVE_BFP_NUMERICAL_MISMATCH bits8 pass0 row0 got([0-9a-f]{8}) expected([0-9a-f]{8});', record)
assert len(matches) == 1
observed, old_expected = (int(part, 16) for part in matches[0])
assert results[8][0] == (old_expected, observed) == (0x4180b340, 0x4180b500)
assert all(ideal == predicted for ideal, predicted in results[4])
assert sum(ideal != predicted for ideal, predicted in results[8]) == 30
print('PASS separate Blackhole arithmetic characterization:all32 BFP4 predictions agree with ideal;BFP8 recorded row0 predicted EXACTLY,30/32 rows predict ideal differences;OTHER BFP8 HARDWARE WORDS UNOBSERVED;strict diagnostic/old observer/Work comparators UNCHANGED;NO GPU OR ACCEPTANCE AUTHORITY')
