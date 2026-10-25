#!/usr/bin/env python3
"""Exact NEW code DATA/ALL120 mappings;no native/card/SDK writes."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify

assert len(sys.argv)==3
image,peer=map(pathlib.Path,sys.argv[1:])
verify(image,peer)
here=pathlib.Path(__file__).resolve().parent
module=runpy.run_path(str(here.parent/'generate_qwen38_resident_layer_images.py'))
for name,(section,size,digest,suffix) in module['PINS'].items():
    data=((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
    if name.startswith('root-'):
        patch=module['generate']([str(image),str(peer),name])
        source=patch.split('\n',2)[2].rsplit('*** End Patch\n',1)[0]
        expected=''.join(line[1:]+'\n' for line in source.splitlines())
        assert (here.parents[2]/'src/add-ons/kernel/drivers/misc'/('PioneerQ38ResidentLayer'+suffix+'.h')).read_text()==expected
with tempfile.TemporaryDirectory(prefix='pioneer-layer-images-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    source=here/'pioneer_q38_resident_layer_images_test.cpp'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']
    subprocess.run([*common,'-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(peer)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    subprocess.run([*common,'-D_KERNEL_MODE','-c',str(source),'-o',str(pathlib.Path(temporary)/'kernel-data-only.o')],check=True)
print('PASS SAME complete immutable DATA _KERNEL_MODE compile;NO native context/ABI/selector or Work grant')
