#!/usr/bin/env python3
"""Synthetic identity, zero-lane, sign and cross-run boundary checks."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='q38-value-attention-',dir='/private/tmp') as directory:
    root=Path(directory);a=root/'value.bin';b=root/'attention.bin';size=64*5120*4
    value=bytearray(size);attention=bytearray(size)
    raw=[0.0 if i%16<4 else (-1.0 if i%2 else 1.0) for i in range(1024)]
    packed=[0 if x==0 else (0xbc00 if x<0 else 0x3c00) for x in raw]
    observed=[(-x if i%256==4 else x) for i in range(6144) for x in [raw[(i//256//6)*256+i%256]]]
    struct.pack_into('<1024f',value,0,*raw);struct.pack_into('<1024H',value,4096,*packed)
    struct.pack_into('<6144f',attention,0,*observed)
    a.write_bytes(value);b.write_bytes(attention)
    sha=lambda data:hashlib.sha256(data).hexdigest()
    command=[sys.executable,str(here/'compare_pioneer_q38_dense_value_attention.py'),str(a),'--value-sha256',sha(value),str(b),'--attention-sha256',sha(attention)]
    result=subprocess.run(command,check=True,text=True,capture_output=True)
    assert 'LAYER_BOUNDARY_MATCH 62/62' in result.stdout
    assert 'raw=64 packed=64 raw_by_lane16=16,16,16,16,0,0,0,0,0,0,0,0,0,0,0,0' in result.stdout
    assert result.stdout.count('eligible=192 opposite_sign=1 raw_nonzero_attention_zero=0')==24
    altered=bytearray(value);altered[2*20480]=1;a.write_bytes(altered)
    assert subprocess.run(command,capture_output=True).returncode!=0
    command[4]=sha(altered)
    result=subprocess.run(command,check=True,text=True,capture_output=True)
    assert 'LAYER_BOUNDARY_MATCH 61/62' in result.stdout
print('PASS synthetic cross-run trace screen and identity refusal; NO DEVICE')
