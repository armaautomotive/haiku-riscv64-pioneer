#!/usr/bin/env python3
"""Reproduce generated DATA, reject corrupt artifacts, verify every L1/code word."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent;root,worker=map(pathlib.Path,sys.argv[1:])
generator=here.parent/'generate_qwen38_mixed_recurrent_block_programs.py'
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-footprint-',dir='/private/tmp') as tmp:
 d=pathlib.Path(tmp);generated=d/'programs.h';binary=d/'test'
 subprocess.run([sys.executable,str(generator),str(root),str(worker),str(generated)],check=True)
 assert generated.read_bytes()==(here.parent/'PioneerQ38MixedRecurrentBlockPrograms.h').read_bytes()
 # Every pinned file (including ELFs,not just used sections) is refused
 # before output creation if corrupt. Symlink original immutable inputs,
 # replace only ONE with a privately corrupted test byte.
 files={root:('boot.bin','text.bin','conv.bin','program.elf'),worker:(
  'boot.bin','mixed.bin','dense.bin','ffn.bin','service.bin','affine.bin','text.bin','gdn.bin','program.elf')}
 refusals=0
 for directory,names in files.items():
  for name in names:
   case=d/('bad'+str(refusals));case.mkdir()
   for path in directory.iterdir():
    if path.is_file():(case/path.name).symlink_to(path)
   target=case/name;target.unlink();data=bytearray((directory/name).read_bytes());assert data;data[len(data)//2]^=1;target.write_bytes(data)
   output=d/('NO_OUTPUT'+str(refusals))
   p=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),str(case if directory==worker else worker),str(output)],capture_output=True)
   assert p.returncode and not output.exists();refusals+=1
 p=subprocess.run([sys.executable,str(generator),str(root),str(worker),str(generated)],capture_output=True)
 assert p.returncode and generated.read_bytes()==(here.parent/'PioneerQ38MixedRecurrentBlockPrograms.h').read_bytes();refusals+=1
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_mixed_recurrent_block_footprint_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 print('PASS recurrent immutable generator/exact reproduction/corruption+replacement refusals'+str(refusals)+';NO device IO or native permission')
