#!/usr/bin/env python3
"""Actual complete-image FFN subroutine+all current peer instructions;NO card."""
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify

if len(sys.argv)!=3:
    raise SystemExit(2)
here=pathlib.Path(__file__).resolve().parent
image,peer=map(pathlib.Path,sys.argv[1:])
verify(image, peer)
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
for section,suffix in (('text','Main'),('prefix','Prefix'),('graph','Graph')):
    words=[int(v,16) for v in re.findall(r'0x([0-9a-fA-F]{8})U',(kernel/('PioneerQ38FfnResidualRoot'+suffix+'.h')).read_text())]
    assert (image/'unchanged-ffn'/(section+'.bin')).read_bytes()==struct.pack('<'+'I'*len(words),*words)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_residual_rms_ffn_cooperative_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-layer-ffn-test-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_resident_layer_ffn_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(peer)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
