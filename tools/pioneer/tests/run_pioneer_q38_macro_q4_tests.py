#!/usr/bin/env python3
"""Local exact decode and complete authenticated worker tests;NO card IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require,stable_bytes

args=sys.argv[1:];mode=[]
for option in ('--pilot','--faults-only'):
 if option in args:
  require(args.count(option)==1 and not mode,'one partial-test mode');args.remove(option);mode=[option]
require(len(args) in (0,2),'optional VECTOR_CONTROL MACRO_CANDIDATE [--pilot|--faults-only]')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'no ambient role override')
here=pathlib.Path(__file__).resolve().parent
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off',
        '-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
environment={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
def pins(path):
 return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
             if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def snapshot(directory,records):
 require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(),'absolute ROM directory')
 require(bool(records),'complete immutable pins required')
 result=[]
 for name,size,digest in records:
  raw=stable_bytes(directory/name,size)
  require(hashlib.sha256(raw).hexdigest()==digest,'whole authenticated ROM '+name);result.append((name,raw))
 return result
with tempfile.TemporaryDirectory(prefix='pioneer-macro-q4-tests-',dir='/private/tmp') as temporary:
 root=pathlib.Path(temporary);binary=root/'arithmetic'
 subprocess.run([*common,str(here/'pioneer_q38_macro_q4_decode_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env=environment)
 if args:
  control,candidate=map(pathlib.Path,args)
  mixed=control/'unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
  snapshots=[snapshot(directory,pins(script)) for directory,script in (
   (mixed,here/'run_pioneer_q38_mixed_worker_tests.py'),
   (control,here.parent/'build_qwen38_vector_q4_metadata_worker.py'),
   (candidate,here.parent/'build_qwen38_macro_q4_decode_worker.py'))]
  legacy=[(name+'.bin',size,digest) for name,size,digest in pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
  legacy.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
  original=snapshot(mixed/'unchanged-worker',legacy)
  tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
  images=[]
  for i,items in enumerate(snapshots):
   directory=root/('AUTHENTICATED_ROM_'+str(i));directory.mkdir(mode=0o700)
   for name,raw in items:
    with (directory/name).open('xb') as stream:stream.write(raw)
    (directory/name).chmod(0o400)
   (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')]))
   (directory/'program.symbols').chmod(0o400);images.append(directory)
  old=images[0]/'unchanged-worker';old.mkdir(mode=0o700)
  for name,raw in original:
   with (old/name).open('xb') as stream:stream.write(raw)
   (old/name).chmod(0o400)
  flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
  binary=root/'worker'
  subprocess.run([*common,*flags,str(here/'pioneer_q38_macro_q4_worker_test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary),*map(str,images),*mode],check=True,env=environment)
print('CLOSED_MACRO_Q4_TESTS local only;NO native/card/Assistant/model-quality/TPS/speedup grant',flush=True)
