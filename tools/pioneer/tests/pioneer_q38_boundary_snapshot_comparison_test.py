#!/usr/bin/env python3
"""SYNTHETIC boundary DATA only,not a hardware/model numerical result."""
import contextlib
import importlib.util
import io
import pathlib
import struct

here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('boundary_compare',here/'compare_pioneer_q38_observed_trace.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
pins=m.CPU_PINS
h=[0x31424851,1,0,248319,1,5120,3,0,0x00010075,0x20261031,9,0,2,0,0,0]
words=h+[0x3f800000]*5120+[0x454e4442]
data=bytearray(struct.pack('<5137I',*words));refused=0
for layer in range(1,65):
    selected=data.copy();struct.pack_into('<I',selected,4*4,layer)
    decoded=m.decode_boundary_snapshot(selected)
    assert decoded['header'][4]==layer and m.difference(decoded,decoded,0,5120)==(0,0,0)
    try:m.decode_hidden_snapshot(selected)
    except ValueError:refused+=1
    else:raise AssertionError('boundary family promoted to strict first-layer DATA')
def reject(value):
    global refused
    try:m.decode_boundary_snapshot(value)
    except ValueError:refused+=1;return
    raise AssertionError('invalid boundary DATA accepted')
for index in range(16):
    if index in (4,10,11):continue
    for bit in range(32):
        bad=data.copy();struct.pack_into('<I',bad,index*4,words[index]^(1<<bit));reject(bad)
for layer in (0,65,0xffffffff):
    bad=data.copy();struct.pack_into('<I',bad,4*4,layer);reject(bad)
bad=data.copy();struct.pack_into('<II',bad,10*4,0,0);reject(bad)
for bad in (data[:-1],data+b'\0',data[:-4]+b'\0'*4):reject(bad)
for value in (0x7fc01234,0x7f800000,0xff800000):
    bad=data.copy();struct.pack_into('<I',bad,16*4,value);reject(bad)
changed=data.copy();struct.pack_into('<I',changed,16*4,0x3f800001)
assert m.difference(m.decode_boundary_snapshot(data),m.decode_boundary_snapshot(changed),0,5120)[0]==1
# The ACTUAL layer controls the expected slice;the fixed CPU corpus/pins
# are never replaced. This exercises main with explicitly MOCK I/O only.
old_load,old_bytes=m.load_cpu,m.stable_bytes
try:
    bit_layers=[];float_layers=[]
    for n in range(65):bit_layers.extend([n]*5120);float_layers.extend([float(n)]*5120)
    m.load_cpu=lambda path:[{'bits':bit_layers,'floats':float_layers}]
    original_difference=m.difference
    seen=[]
    def check_slice(expected,actual,first,count):
        layer=actual['header'][4];assert expected['bits']==[layer]*5120 and expected['floats']==[float(layer)]*5120
        seen.append(layer);return (1,1.0,1.0) # MUST produce exit1,not a fabricated pass
    m.difference=check_slice
    for layer in (1,4,32,64):
        selected=data.copy();struct.pack_into('<I',selected,4*4,layer)
        m.stable_bytes=lambda path,size,selected=selected:bytes(selected)
        with contextlib.redirect_stdout(io.StringIO()):assert m.main(['/MOCK_CPU','/MOCK_ACTUAL','--held-boundary-only'])==1
    assert seen==[1,4,32,64]
finally:
    m.load_cpu,m.stable_bytes,m.difference=old_load,old_bytes,original_difference
try:m.main(['/NEVER_CPU','/NEVER_FILE','--held-boundary-only','--diagnostic-only'])
except ValueError as error:assert 'never permits diagnostic-only' in str(error);refused+=1
else:raise AssertionError('boundary diagnostic-only accepted')
with contextlib.redirect_stderr(io.StringIO()):
    try:m.main(['/NEVER_CPU','/NEVER_FILE','--held-boundary-only','--held-hidden-only'])
    except SystemExit as error:assert error.code==2;refused+=1
    else:raise AssertionError('boundary/strict first-layer modes mixed')
assert m.CPU_PINS==pins
print(f'PASS observed boundary parser/all64 layers/{refused} family+header+range+nonfinite+mode refusals/actual-layer expected slice checked through MOCK I/O/main returns1 on differences/unchanged production CPU pins;NO hardware/numerical/Work/TPS proof')
