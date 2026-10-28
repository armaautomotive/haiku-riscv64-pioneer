#!/usr/bin/env python3
"""Pinned NEW QGC1 + SAME fresh QGW1 actual instructions. NEVER hardware."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==4 or len(sys.argv)==5 and sys.argv[4] in ('--refusals','--prepare-only')
image,old,worker=map(pathlib.Path,sys.argv[1:4]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (image,old,worker))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
for directory,script in ((image,here.parent/'build_qwen38_global_chain_root.py'),(old,here/'run_pioneer_q38_mixed_worker_tests.py'),(worker,here.parent/'build_qwen38_global_worker_image.py')):
 tree=ast.parse(script.read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
 if directory==image:
  unrolled=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='UNROLLED_PINS' for t in n.targets))
  sets=[(name,p) for name,p in (('baseline-seals',pins),('unrolled-seals',unrolled)) if p and all(
   len(data:=(directory/file).read_bytes())==size and hashlib.sha256(data).hexdigest()==digest for file,size,digest in p)]
  assert len(sets)==1,'ROOT must match exactly ONE literal-pinned build variant'
  variant,pins=sets[0];print('PINNED QGC1 ROOT '+variant+';not native/physical/Work qualification',flush=True)
 assert pins
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
assert subprocess.check_output([nm,'-n',str(worker/'program.elf')],text=True)==(worker/'program.symbols').read_text(),'worker wait/STOP metadata must derive from the literal-pinned ELF'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-global-chain-instructions-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test';intervals={}
 symbols=subprocess.check_output([nm,'-S','-n','--demangle',str(image/'program.elf')],text=True)
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
 profile=pathlib.Path(tmp)/'profile.map';profile.write_text(''.join(format(begin,'x')+' '+format(size,'x')+' '+name+'\n' for (begin,size),name in ordered))
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_global_chain_instruction_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1','PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
