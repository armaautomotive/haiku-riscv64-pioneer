#!/usr/bin/env python3
"""NEW complete-layer geometry ONLY;NO kernel mapping/card/IO access."""
import os
import pathlib
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify

assert len(sys.argv)==3
image,peer=map(pathlib.Path,sys.argv[1:])
verify(image,peer)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-layer-footprint-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']
    source=here/'pioneer_q38_resident_layer_footprint_test.cpp'
    subprocess.run([*common,'-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(peer)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    subprocess.run([*common,'-D_KERNEL_MODE','-c',str(source),'-o',str(pathlib.Path(name)/'kernel-data-only.o')],check=True)
print('PASS same pure geometry _KERNEL_MODE compile;NO native adapter/selector or code/data ownership authority')
