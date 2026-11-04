#!/usr/bin/env python3
"""Independent exact-rational check of SYNTHETIC original Q4 operand outputs."""
import fractions
import pathlib
import re
import subprocess
import sys

assert __debug__ and len(sys.argv) == 2
binary = pathlib.Path(sys.argv[1])
assert binary.is_absolute() and binary.is_file()


def power(exponent):
    return fractions.Fraction(1 << exponent) if exponent >= 0 else fractions.Fraction(1, 1 << -exponent)


def half(encoded):
    exponent, mantissa = (encoded >> 10) & 31, encoded & 1023
    assert exponent != 31
    return (-1 if encoded & 0x8000 else 1) * (1024 + mantissa if exponent else mantissa) * power(exponent - 25 if exponent else -24)


def binary32(value, zero_sign):
    if not value:
        return zero_sign << 31
    sign = int(value < 0)
    magnitude = abs(value)
    exponent = magnitude.numerator.bit_length() - magnitude.denominator.bit_length()
    scaled = magnitude / power(exponent - 23)
    integer, remainder = divmod(scaled.numerator, scaled.denominator)
    twice = 2 * remainder - scaled.denominator
    integer += int(twice > 0 or (twice == 0 and integer & 1))
    if integer == 1 << 24:
        integer >>= 1
        exponent += 1
    assert (1 << 23) <= integer < (1 << 24) and -126 <= exponent <= 127
    return (sign << 31) | ((exponent + 127) << 23) | (integer - (1 << 23))


closed = subprocess.run([str(binary), '--fraction-fixture'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
assert not closed.stderr
count = zero_count = 0
for line in closed.stdout.decode('ascii').splitlines():
    match = re.fullmatch(r'Q4_FPU_FRACTION ([0-9a-f]{4}) ([0-9a-f]{4}) (\d+) (\d+) (\d+) ([0-9a-f]{8})', line)
    assert match
    d, dmin = (int(x, 16) for x in match.group(1, 2))
    scale, minimum, quant = (int(x) for x in match.group(3, 4, 5))
    assert scale < 64 and minimum < 64 and quant < 16
    product, subtract = half(d) * scale * quant, half(dmin) * minimum
    # Products contain at most21 significand bits and are exact binary32.
    # For a zero difference, IEEE RNE cancellation is+0 unless BOTH input
    # operands are zero with negative signs (-0 + -0). Fractions have no-0.
    zero_sign = int(not product and not subtract and bool(d & 0x8000) and not (dmin & 0x8000))
    bits = binary32(product - subtract, zero_sign)
    high, tail = divmod(bits, 1 << 16)
    high += int(tail > 32768 or (tail == 32768 and high & 1))
    expected = high << 16
    actual = int(match.group(6), 16)
    assert actual == expected, (line, hex(expected))
    count += 1
    zero_count += int(not (bits & 0x7fffffff))
assert count == 16384 and zero_count >= 64
print(f'PASS independent Fraction Q4 decode:{count} synthetic outputs,{zero_count} signed-zero/cancellation cases;no CXX half/FMA/rounding reference shared;NO hardware/quality/speed proof')
