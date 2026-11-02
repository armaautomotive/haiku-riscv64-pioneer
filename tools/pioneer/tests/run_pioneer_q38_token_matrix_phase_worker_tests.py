#!/usr/bin/env python3
"""Actual frozen complete sampler worker vs unchanged e9ef,local endpoints."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require,stable_bytes

require(len(sys.argv)==3,'CONSTANT_Q4 MATRIX_PHASE')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'no role overrides')
here=pathlib.Path(__file__).resolve().parent
control,image=map(pathlib.Path,sys.argv[1:]);wide=control/'unchanged-wide'
mixed=wide/'unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
def pins(script):
 return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
  if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def snapshot(directory,records):
 require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(),'absolute frozen ROM directory')
 require(bool(records),'immutable pins required');items=[]
 for name,size,digest in records:
  raw=stable_bytes(directory/name,size);require(hashlib.sha256(raw).hexdigest()==digest,'whole ROM '+name)
  items.append((name,raw))
 return items
snapshots=[snapshot(directory,pins(script)) for directory,script in (
 (mixed,here/'run_pioneer_q38_mixed_worker_tests.py'),
 (control,here.parent/'build_qwen38_q4_constant_issue_worker_image.py'),
 (image,here.parent/'build_qwen38_token_matrix_phase_worker.py'))]
legacy_records=[(name+'.bin',size,digest) for name,size,digest in pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy_records.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy_snapshot=snapshot(mixed/'unchanged-worker',legacy_records)
# Run the complete unchanged e9ef-vs70f instructions/fault/retained controls
# FIRST;no old command expectations or fixture bounds are relaxed.
subprocess.run([sys.executable,str(here/'run_pioneer_q38_q4_constant_issue_worker_tests.py'),str(wide),str(control)],check=True)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-matrix-phase-execution-',dir='/private/tmp') as temporary:
 root=pathlib.Path(temporary);images=[]
 for index,items in enumerate(snapshots):
  directory=root/('AUTHENTICATED_ROM_'+str(index));directory.mkdir(mode=0o700)
  for name,raw in items:
   path=directory/name
   with path.open('xb') as stream:stream.write(raw)
   path.chmod(0o400)
  (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')]))
  images.append(directory)
 legacy=images[0]/'unchanged-worker';legacy.mkdir(mode=0o700)
 for name,raw in legacy_snapshot:
  path=legacy/name
  with path.open('xb') as stream:stream.write(raw)
  path.chmod(0o400)
 binary=root/'worker-test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_token_matrix_phase_worker_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*map(str,images)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('CLOSED_MATRIX_PHASE_ACTUAL_WORKER_SYNTHETIC_CLOCK_NOT_PHYSICAL_TIMING_OR_DEPLOYMENT',flush=True)
