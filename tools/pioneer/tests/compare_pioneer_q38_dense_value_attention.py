#!/usr/bin/env python3
"""Compare closed value and attention traces; this is not a device test."""
import argparse
import hashlib
import math
import os
from pathlib import Path
import re
import struct

SIZE=64*5120*4
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('value',type=Path);p.add_argument('--value-sha256',required=True)
p.add_argument('attention',type=Path);p.add_argument('--attention-sha256',required=True)
args=p.parse_args()
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('role override refused')
def checked(path,digest):
    if not re.fullmatch('[0-9a-f]{64}',digest) or not path.is_absolute() or path.is_symlink():
        raise SystemExit('invalid identity: '+str(path))
    data=path.read_bytes()
    if len(data)!=SIZE or hashlib.sha256(data).hexdigest()!=digest:
        raise SystemExit('identity mismatch: '+str(path))
    return data
value=checked(args.value,args.value_sha256);attention=checked(args.attention,args.attention_sha256)
raw=struct.unpack_from('<1024f',value);packed=struct.unpack_from('<1024H',value,4096)
observed=struct.unpack_from('<6144f',attention)
if not all(math.isfinite(x) for x in raw+observed) or any(x&0x7c00==0x7c00 for x in packed):
    raise SystemExit('nonfinite input refused')
# The attention capture overwrites the first 6144 floats, including part of
# slot 1; compare only the intact layer slots 2..63 across the two runs.
same=sum(value[i*20480:(i+1)*20480]==attention[i*20480:(i+1)*20480] for i in range(2,64))
print(f'LAYER_BOUNDARY_MATCH {same}/62; cross-run equality is not same-run provenance')
for group in range(4):
    r=raw[group*256:(group+1)*256];v=packed[group*256:(group+1)*256]
    print(f'VALUE_ZERO group={group} raw={sum(x==0 for x in r)} packed={sum((x&0x7fff)==0 for x in v)} '
          f'raw_by_lane16={",".join(str(sum(r[i]==0 for i in range(lane,256,16))) for lane in range(16))}')
    for head in range(group*6,group*6+6):
        a=observed[head*256:(head+1)*256]
        eligible=sum(x!=0 and y!=0 for x,y in zip(r,a))
        opposite=sum((x<0)!=(y<0) for x,y in zip(r,a) if x!=0 and y!=0)
        nonzero_to_zero=sum(x!=0 and y==0 for x,y in zip(r,a))
        print(f'ATTENTION_CROSS_RUN group={group} head={head} eligible={eligible} opposite_sign={opposite} '
              f'raw_nonzero_attention_zero={nonzero_to_zero}')
print('CROSS-RUN SCREEN ONLY; no capture identity, cause, numerical correctness or speed claim')
