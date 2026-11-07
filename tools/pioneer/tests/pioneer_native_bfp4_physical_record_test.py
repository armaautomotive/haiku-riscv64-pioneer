#!/usr/bin/env python3
"""Verify saved BFP4 K5120 record against frozen independent expectations."""
import hashlib
from pathlib import Path
import re

assert __debug__
here=Path(__file__).resolve().parent
record=here.parent/'benchmarks/qwen38-bfp4-band-physical-20261106'
expected_hashes={
 'band5120.stdout':'0995d5a194ec5622107213051ffb2151e065e1aec7505ae595aa2e1259f50567',
 'band5120.stderr':'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
 'band5120.exit':'9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa',
 'state':'92aee7633ea871aef80ac2624514ea161eafff5c151494fbe44a837483147143'}
for name,digest in expected_hashes.items():
    data=(record/name).read_bytes()
    assert hashlib.sha256(data).hexdigest()==digest,(name,digest)
assert (record/'band5120.exit').read_text()=='0\n'
assert (record/'state').read_text()=='STABLE041_RESTORED_FULL_K5120_BFP4_EXACT64_PASSED\n'
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
assert len(lines)==65
for index,line in enumerate(lines[:64]):
    match=re.fullmatch(r'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})',line)
    assert match and int(match[1])==index//32 and int(match[2])==index%32
    assert match[3]==values[index%32] and match[4]==values[index%32]
assert lines[64]=='PASS_NATIVE_BFP_FULL_K bits4 width5120 rows32 dual_jobs2 phases2 exact_words64 polls2 generation13;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
print('PASS saved actual BFP4 K5120 record: all 64 words, exact hashes, closed result and restored state; synthetic correctness only, no speed or model-quality claim')
