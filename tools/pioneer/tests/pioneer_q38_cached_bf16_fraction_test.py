#!/usr/bin/env python3
"""Independent exact-rational nearest-BF16 check of emitted RV32 producer."""
from fractions import Fraction
import pathlib
import re
import sys
assert __debug__ and len(sys.argv)==2
def power(n):return Fraction(2**n) if n>=0 else Fraction(1,2**-n)
def fp32(bits):
 sign=-1 if bits>>31 else 1;exponent=bits>>23&255;mantissa=bits&0x7fffff
 assert exponent!=255
 return sign*(mantissa+(8388608 if exponent else 0))*power(exponent-150 if exponent else -149)
def bf16(bits):
 sign=-1 if bits>>15 else 1;exponent=bits>>7&255;mantissa=bits&127
 assert exponent!=255
 return sign*(mantissa+(128 if exponent else 0))*power(exponent-134 if exponent else -133)
cases=0
for line in pathlib.Path(sys.argv[1]).read_text().splitlines():
 if not line.startswith('ACTUAL_BF16_PRODUCER '):continue
 m=re.fullmatch(r'ACTUAL_BF16_PRODUCER bits([0-9a-f]{8}) value([0-9a-f]{4})',line);assert m
 bits,value=map(lambda x:int(x,16),m.groups());low=bits>>16;upper=low+1
 a,b=abs(fp32(bits)-bf16(low)),abs(fp32(bits)-bf16(upper))
 expected=low if a<b else upper if b<a else low if not low&1 else upper
 assert value==expected
 cases+=1
assert cases==128
print('PASS independent Fraction nearest-BF16128 actual RV32 producer lanes/ties/negative values;NO shared CXX bit-rounder or native/quality/speed authority')
