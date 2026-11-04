#!/usr/bin/env python3
"""Independent exact-rational Q6_K/Q8_0 BF16 operand check,not device proof."""
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


def bf16(value, zero_sign):
    if not value:
        return zero_sign << 31
    sign = int(value < 0)
    value = abs(value)
    exponent = value.numerator.bit_length() - value.denominator.bit_length()
    scaled = value / power(exponent - 7)
    integer, remainder = divmod(scaled.numerator, scaled.denominator)
    integer += int(2 * remainder > scaled.denominator or (2 * remainder == scaled.denominator and integer & 1))
    if integer == 256:
        integer >>= 1
        exponent += 1
    assert 128 <= integer < 256 and -126 <= exponent <= 127
    return ((sign << 15) | ((exponent + 127) << 7) | (integer - 128)) << 16


closed = subprocess.run([str(binary), '--fraction-fixture'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
assert not closed.stderr
counts = {6: 0, 8: 0}
zeros = 0
for line in closed.stdout.decode('ascii').splitlines():
    match = re.fullmatch(r'MIXED_FPU_FRACTION ([68]) ([0-9a-f]{4}) (-?\d+) (-?\d+) ([0-9a-f]{8})', line)
    assert match
    kind, encoded, scale, quant, actual = (int(match.group(i), 16 if i in (2, 5) else 10) for i in range(1, 6))
    assert (-128 <= scale <= 127 and -32 <= quant <= 31) if kind == 6 else (scale == 1 and -128 <= quant <= 127)
    # Products are exact binary32. XOR includes signed half zero,negative
    # scale and negative quant;integer0 is ALWAYS positive zero.
    zero_sign = bool(encoded & 0x8000) ^ (scale < 0) ^ (quant < 0)
    expected = bf16(half(encoded) * scale * quant, zero_sign)
    assert actual == expected, (line, hex(expected))
    counts[kind] += 1
    zeros += not (expected & 0x7fffffff)
assert counts == {6: 32768, 8: 8192} and zeros >= 2048
print(f'PASS independent Fraction mixed FPU decode:{counts[6]} Q6/{counts[8]} Q8 outputs,{zeros} signed-zero cases;direct rational-to-BF16 rounding,no shared CXX half/FMA/rounding oracle;NO hardware/quality/speed proof')
