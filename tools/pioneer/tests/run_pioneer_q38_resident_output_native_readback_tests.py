#!/usr/bin/env python3
"""Actual SAME native output owner+fixed observer+programs;NO physical card."""
import hashlib
import pathlib
import runpy
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import HERE,compile_test,run_test

if len(sys.argv) not in (7,8):
    raise SystemExit(2)
mode=sys.argv[7] if len(sys.argv)==8 else '--all'
assert mode in ('--all','--positive')
image=pathlib.Path(sys.argv[6]);peer=image/'exact-companion'
for name,(section,size,digest,_) in runpy.run_path(str(HERE.parent/'generate_qwen38_resident_output_images.py'))['PINS'].items():
    data=((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-output-native-readback-',dir='/private/tmp') as temporary:
    binary=compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_native_readback_test','pioneer_q38_resident_output_execution_machine'],
                        sys.argv[1:6],producer=False,native_projection=True)
    run_test(binary,[*sys.argv[1:6],str(image),str(peer),mode])
