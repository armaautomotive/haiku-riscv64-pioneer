#!/usr/bin/env python3
"""Full worker + returning producer/FPU OFFBOARD qualification,not inference."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
paired=sys.argv[-1:]==['--pair-issue']
if paired:sys.argv.pop()
assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
sdk,original,private,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,original,private,out)) and sdk.is_dir() and original.is_file() and private.is_dir()
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);exits={};commands=[];tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx=os.environ.get('CXX','c++');dependencies={Path(__file__).resolve()}
for name in ('build_q38_bf16_worker.py','blackhole_q38_bf16_worker_start.S','build_qwen38_global_worker_image.py'):
 dependencies.add(here.parent/name)
for name in ('run_pioneer_q38_bf16_worker_tests.py','run_pioneer_q38_ffn_persistent_image_tests.sh','pioneer_q38_cached_bf16_fraction_test.py','run_pioneer_q38_cached_fpu_tests.py'):
 dependencies.add(here/name)
def collect(compiler,unit,flags=()):
 data=subprocess.check_output([str(compiler),'-std=c++17',*flags,'-MM',str(unit)],text=True)
 dependencies.update(Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split())
for name in ('blackhole_qwen38_global_worker.cpp','blackhole_q38_bf16_persistent_producer.cpp'):
 collect(tool+'g++',here.parent/name,['-march=rv32imf','-mabi=ilp32','-DPIONEER_Q38_BF16_WORKER_IMAGE=1'])
collect(tool+'g++',here.parent/'blackhole_q38_cached_fpu_matrix.cpp',['-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_CACHED_FPU_MATRIX_IMAGE=1','-DPIONEER_Q38_CACHED_FPU_RETURN_ONLY=1',*(['-DPIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE=1'] if paired else [])])
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
collect(cxx,here/'pioneer_q38_bf16_worker_test.cpp',flags)
for name in ('pioneer_q38_bf16_worker_probe.cpp','pioneer_q38_sigmoid_precision_host_bridge.cpp','pioneer_q38_bf16_worker_fpu_test.cpp','pioneer_q38_bf16_producer_fpu_test.cpp','pioneer_q38_paired_projection_probe.cpp'):
 collect(cxx,here/name)
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False)
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_FULL_BF16_WORKER_OFFBOARD',name,result.returncode,flush=True)
extra=['--pair-issue'] if paired else []
run('build',[sys.executable,here.parent/'build_q38_bf16_worker.py',sdk,out/'worker',*extra])
run('producer',[sys.executable,here/'run_pioneer_q38_bf16_worker_tests.py',out/'worker/unchanged-global/qualified-math',out/'worker',out/'producer-tests'])
run('cached-regression',[sys.executable,here/'run_pioneer_q38_cached_fpu_tests.py',sdk,original,private,out/'cached-regression',*extra])
run('fpu-compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
 here/'pioneer_q38_bf16_worker_fpu_test.cpp',here/'pioneer_q38_paired_projection_probe.cpp','-o',out/'fpu-test'])
run('fpu',[out/'fpu-test',out/'worker',out/'cached-regression/services/bf16_producer',out/'cached-regression/services/cached_fpu_matrix'])
run('connected-compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
 here/'pioneer_q38_bf16_producer_fpu_test.cpp',here/'pioneer_q38_paired_projection_probe.cpp','-o',out/'connected-test'])
run('connected',[out/'connected-test',out/'worker',out/'cached-regression/services/cached_fpu_matrix'])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_FULL_BF16_WORKER_OFFBOARD children'+str(len(exits))+'_stderr0_source'+str(len(frozen))+
 '_NO_CONNECTED_MODEL_ROOT_NATIVE_HARDWARE_FULL_MODEL_QUALITY_OR_TPS_AUTHORITY',flush=True)
