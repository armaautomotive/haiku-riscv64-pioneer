#!/usr/bin/env python3
"""Compare immutable token-0 HOST-native DATA; NEVER full-run/physical proof."""
import array
import hashlib
import json
import math
import pathlib
import struct
import sys
from compare_pioneer_q38_observed_trace import difference, load_cpu, require, stable_bytes

PINS = {
    'hidden': (65*5120, '5a1c8478fea72b7a3d1acc63eecdebdb3fbdcf3c1f57166e04a3a19aa99ab784'),
    'logits': (248320, '81c3bfed510fdd8a74200250dba3887f3fc6596c9ebe2fbb69b796e0af9e37da'),
    'record': (64, '3ddd040aea71afba0434ea81f9e3be6ecf4280dc6b4260281f3f17081dc248d0'),
}
def main():
    require(len(sys.argv)==3, 'CPU_DIRECTORY PARTIAL_HOST_NATIVE_DIRECTORY')
    cpu_path,partial=map(pathlib.Path,sys.argv[1:])
    require(cpu_path.is_absolute() and partial.is_absolute(), 'absolute inputs')
    cpu=load_cpu(cpu_path)[0]
    meta=json.loads((partial/'metadata.json').read_text())
    require(meta['scope']=='PARTIAL_NATIVE_HOST_EMULATION_DATA_ONLY_NOT_FULL_RUN_STOP_END_PHYSICAL_OR_TPS', 'partial scope')
    require(meta['input']==248319 and meta['layers']==64 and meta['rows']==248320 and meta['committed']==1, 'actual token0 metadata')
    require(meta['guest_state_writes']==meta['injected_products']==0, 'no target writes or supplied products')
    parts={}
    for name,(count,digest) in PINS.items():
        payload=stable_bytes(partial/(name+'.raw'),count*4)
        require(hashlib.sha256(payload).hexdigest()==digest and meta[name]==dict(words=count,sha256=digest), 'original partial '+name)
        parts[name]=payload
    words=array.array('I');words.frombytes(parts['hidden']+parts['logits'])
    floats=array.array('f');floats.frombytes(parts['hidden']+parts['logits'])
    require(sys.byteorder=='little' and words.itemsize==floats.itemsize==4 and all(math.isfinite(x) for x in floats), 'finite binary32 DATA')
    actual=dict(bits=words,floats=floats)
    record=struct.unpack('<64I',parts['record'])
    require(record[:12]==(0x31435451,1,0,248319,meta['selected'],words[65*5120+meta['selected']],1,1,1,1,64,973), 'actual record family/input/greedy/logit')
    first=None
    for boundary in range(65):
        diff,maximum,relative=difference(cpu,actual,boundary*5120,5120)
        print('PARTIAL_HOST_NATIVE_TOKEN0_BOUNDARY',boundary,'bit_differences',diff,'max_abs',maximum,'relative_l2',relative)
        if diff and first is None:first=boundary
    logits=difference(cpu,actual,65*5120,248320)
    print('PARTIAL_HOST_NATIVE_TOKEN0_VOCAB',logits)
    print('TOP_CPU',hex(cpu['header'][6]),'TOP_PARTIAL_HOST_NATIVE',hex(record[5]))
    print('FIRST_DIFFERENT_BOUNDARY',first,'NO_PHYSICAL_STOP_END_FULL_RUN_WORK_OR_TPS_QUALIFICATION')
    return int(first is not None or logits[0]!=0)
if __name__=='__main__':
    raise SystemExit(main())
