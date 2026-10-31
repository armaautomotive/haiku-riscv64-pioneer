#!/usr/bin/env python3
"""SYNTHETIC snapshot format/metrics only. Never change the fixed CPU pins."""
import contextlib
import importlib.util
import io
import pathlib
import struct

here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('hidden_compare',here/'compare_pioneer_q38_observed_trace.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
original_pins=module.CPU_PINS
h=[0x31444851,1,0,248319,1,5120,3,0,0x00010075,0x20261031,4,0,2,0,0,0]
words=h+[0x3f800000]*5120+[0x454e4448]
data=bytearray(struct.pack('<5137I',*words))
expected=module.decode_hidden_snapshot(data)
assert module.difference(expected,expected,0,5120)==(0,0,0)
changed=data.copy();struct.pack_into('<I',changed,(16+73)*4,0x3f800001)
actual=module.decode_hidden_snapshot(changed)
count,maximum,relative=module.difference(expected,actual,0,5120)
assert count==1 and maximum==2**-23 and relative>0
zero=data.copy();struct.pack_into('<I',zero,16*4,0)
negative=zero.copy();struct.pack_into('<I',negative,16*4,0x80000000)
assert module.difference(module.decode_hidden_snapshot(zero),module.decode_hidden_snapshot(negative),0,5120)==(1,0,0)
refused=0
def reject(value):
    global refused
    try:module.decode_hidden_snapshot(value)
    except ValueError:refused+=1;return
    raise AssertionError('malformed synthetic snapshot accepted')
for index in range(16):
    if index in (10,11):continue # actual generation can be any nonzero64-bit value
    for bit in range(32):
        bad=data.copy();struct.pack_into('<I',bad,index*4,words[index]^(1<<bit));reject(bad)
bad=data.copy();struct.pack_into('<II',bad,10*4,0,0);reject(bad)
bad=data.copy();struct.pack_into('<II',bad,10*4,0,1);module.decode_hidden_snapshot(bad)
for bad in (data[:-1],data+b'\0',data[:-4]+b'\0'*4):reject(bad)
for value in (0x7fc01234,0x7f800000,0xff800000):
    bad=data.copy();struct.pack_into('<I',bad,16*4,value);reject(bad)
for decoder in (module.decode_prefix,lambda x:module.decode_trace(x,0,True)):
    try:decoder(data)
    except ValueError:refused+=1
    else:raise AssertionError('held snapshot promoted to embedding/full token trace')
never=str(here/'HIDDEN_COMPARATOR_MUST_NOT_OPEN')
try:module.main([never,never,'--held-hidden-only','--diagnostic-only'])
except ValueError as error:assert 'never permits diagnostic-only' in str(error);refused+=1
else:raise AssertionError('held snapshot permitted diagnostic-only acceptance')
with contextlib.redirect_stderr(io.StringIO()):
    try:module.main([never,never,'--held-hidden-only','--first-layer-only'])
    except SystemExit as error:assert error.code==2;refused+=1
    else:raise AssertionError('incompatible snapshot/prefix modes accepted')
assert module.CPU_PINS==original_pins and len(module.CPU_PINS)==3
print(f'PASS held hidden snapshot/SYNTHETIC exact+oneULP+signed-zero strict-bit metrics/{refused} malformed+nonfinite+wrong-family+late-control refusals/no invented embedding/full trace/no diagnostic-only acceptance/production CPU pins unchanged;NO real-card/numerical/Work/TPS qualification')
