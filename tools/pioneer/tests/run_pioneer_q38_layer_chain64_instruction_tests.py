#!/usr/bin/env python3
"""Pinned full64 actual instructions, shared119+byte fabric; NOT hardware."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 or len(sys.argv)==4 and sys.argv[3] in ('--refusals','--prepare-only')
image,worker=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert image.is_absolute() and worker.is_absolute()
for directory,script,key in ((image,here.parent/'build_qwen38_layer_chain_root.py','PINS64'),
 (worker,here/'run_pioneer_q38_mixed_worker_tests.py','PINS')):
 tree=ast.parse(script.read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-layer-chain64-instructions-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 # Derive diagnostic symbol intervals ONLY from the already PINNED ELF.
 # No symbol-map byte is used to select code or change execution state.
 nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
 symbols=subprocess.check_output([nm,'-S','-n','--demangle',str(image/'program.elf')],text=True)
 intervals={}
 for line in symbols.splitlines():
  p=line.split(None,3)
  if len(p)!=4 or p[2] not in ('T','t','W','w'):continue
  begin,size=int(p[0],16),int(p[1],16)
  if not size:continue
  assert not (begin|size)&3
  assert (0x40000<=begin and begin+size<=0x60000) or (0x64000<=begin and begin+size<=0x6c000) or (begin==0x6000 and size==4) or (0x60000<=begin and begin+size<=0x60008)
  intervals.setdefault((begin,size),p[3])
 ordered=sorted(intervals.items());end=0
 for (begin,size),name in ordered:assert begin>=end;end=begin+size
 profile=pathlib.Path(tmp)/'profile.map'
 profile.write_text(''.join(format(begin,'x')+' '+format(size,'x')+' '+name+'\n' for (begin,size),name in ordered))
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_layer_chain64_instruction_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1','PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
