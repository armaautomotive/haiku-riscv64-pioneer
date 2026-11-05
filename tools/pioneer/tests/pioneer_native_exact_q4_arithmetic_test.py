#!/usr/bin/env python3
"""OFFBOARD documented Blackhole lane arithmetic,not high-precision ideal FPU.

Reuse the existing independent integer/Fraction ISA characterization
UNCHANGED,including its old hardware-word regression. No device or Work.
"""
from fractions import Fraction
import pathlib
import runpy
import struct
if not __debug__:
    raise SystemExit('Assertions required')
here=pathlib.Path(__file__).resolve().parent
isa=runpy.run_path(str(here/'pioneer_native_bfp_arithmetic_test.py'))
instruction=isa['instruction']
encode=isa['encode_normal']
def bf16(value):
    bits=struct.unpack('<I',struct.pack('<f',value))[0]
    assert not bits&65535
    return bits>>16
def exact(q,x):
    dst=0
    for values in ([min(v,14)-7 for v in q],[7]*16,[int(v==15) for v in q]):
        dst=instruction(list(map(bf16,values)),list(map(bf16,x)),0,dst)
    assert dst==encode(Fraction(sum(a*b for a,b in zip(q,x))))
checks=0
for q in range(16):
    for x in range(-128,128):
        exact([q]*16,[x]*16);checks+=1
for seed in range(256):
    x=[((seed*7+k*13)&255)-128 for k in range(16)]
    for n in range(16):
        exact([(seed+k*17+n*3)&15 for k in range(16)],x);checks+=1
# EXACT fixed native fixture,independent of its byte packer and client code.
for n in range(16):
    exact([(k*17+n*3+7)&15 for k in range(16)],[-128 if k==0 else ((k*37+11)&255)-128 for k in range(16)]);checks+=1
print(f'PASS_DOCUMENTED_BLACKHOLE_EXACT_Q4_INTEGER_ARITHMETIC checks{checks};threePhase0ProductsLaneAlignmentRoundingAndFP32Accumulation;oldCharacterizationUNCHANGED;NO_HARDWARE_OR_TPS_AUTHORITY')
