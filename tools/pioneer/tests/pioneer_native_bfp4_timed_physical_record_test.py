#!/usr/bin/env python3
"""Verify the archived physical BFP4 timing and correctness record."""
import hashlib
from pathlib import Path
import re

assert __debug__
here=Path(__file__).resolve().parent
record=here.parent/'benchmarks/qwen38-bfp4-band-timing-physical-20261106'
pins={
 'band5120.stdout':'df10a67284455c879fb0ee9661fd21bdd0007cd99c0d3686b2171c940843a3f6',
 'band5120.stderr':'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
 'band5120.exit':'9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa',
 'state':'2d5aeef6d44456423b2ca39d8132932e3c4dcc038b02ee1af82da21ea7a1bfe4'}
for name,digest in pins.items():
    assert hashlib.sha256((record/name).read_bytes()).hexdigest()==digest,name
assert (record/'band5120.exit').read_text()=='0\n'
assert (record/'state').read_text()=='STABLE041_RESTORED_TIMED_BFP4_EXACT64_PASSED\n'
assert re.fullmatch(r'[1-9][0-9]*\n',(record/'parent.pid').read_text())
reference=(here.parent/'pioneer_native_bfp4_band_reference_5120.txt').read_bytes()
assert hashlib.sha256(reference).hexdigest()=='3448d29dc93451b95262466950541b33f013dd41f30db9275a09dbffcdd296c9'
values=[]
for row,line in enumerate(reference.decode().splitlines()):
    match=re.fullmatch(r'row([0-9]+) ([0-9a-f]{8})',line)
    assert match and int(match[1])==row
    values.append(match[2])
assert len(values)==32
lines=(record/'band5120.stdout').read_text().splitlines()
assert len(lines)==67
for i,line in enumerate(lines[:64]):
    match=re.fullmatch(r'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})',line)
    assert match and int(match[1])==i//32 and int(match[2])==i%32
    assert match[3]==values[i%32] and match[4]==values[i%32]
measurements=[]
for p,line in enumerate(lines[64:66]):
    match=re.fullmatch(r'NATIVE_BFP4_BAND_TIMING width5120 pass([01]) start([0-9]+) end([0-9]+) cycles([0-9]+);CARD_CYCLES_ONLY_L1_INPUTS',line)
    assert match and int(match[1])==p
    start,end,cycles=map(int,match.groups()[1:])
    assert start>0 and end>start and cycles==end-start
    measurements.append((start,end,cycles))
assert measurements==[(1078,25857,24779),(26452,51255,24803)]
assert measurements[1][0]>measurements[0][1]
assert lines[66]=='PASS_NATIVE_BFP_FULL_K bits4 width5120 rows32 dual_jobs2 phases2 exact_words64 polls2 generation15;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
print('PASS physical BFP4 timed record: 64 exact words, cycles 24779/24803, closed and restored; L1 input, no model-speed claim')
