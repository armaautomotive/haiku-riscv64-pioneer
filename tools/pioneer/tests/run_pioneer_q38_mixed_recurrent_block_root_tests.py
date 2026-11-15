#!/usr/bin/env python3
"""Actual complete recurrent ROOT plus retained119 workers. No device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3] in ('--faults','--full','--late','--lifecycle','--redistributed','--redistributed-cohort'))
here=pathlib.Path(__file__).resolve().parent
root,worker=map(pathlib.Path,sys.argv[1:3])
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',62404,'28ce6c204ec35cdbeb9700230a6a268d2081d434615efc37024dfb5b2d2b97b0'),
 ('conv.bin',7500,'705770a711c5c738c8e377c68ecb26c27095550bc9801d3089cf1e758a2940c0'),
 ('program.elf',87800,'4b8fb118365a1f72688a7683cacc4786994930593bcdc15309b22ebefa943645')))
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin_name='REDISTRIBUTED_PINS' if len(sys.argv)==4 and sys.argv[3] in ('--redistributed','--redistributed-cohort') else 'PINS'
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==pin_name for t in n.targets)))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-recurrent-block-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 lifecycle=sys.argv[3:]==['--lifecycle']
 source='pioneer_q38_mixed_recurrent_block_lifecycle_test.cpp' if lifecycle else 'pioneer_q38_mixed_recurrent_block_root_test.cpp'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/source),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker),*([] if lifecycle else sys.argv[3:])],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
