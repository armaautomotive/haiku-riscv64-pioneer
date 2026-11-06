#!/usr/bin/env python3
"""Qualify actual local live-BF16 producer code, never inference/device IO."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx=os.environ.get('CXX','c++');unit=here/'pioneer_q38_bf16_activation_test.cpp'
dependencies={Path(__file__).resolve(),here/'pioneer_q38_cached_bf16_fraction_test.py',
 here.parent/'build_q38_bf16_activation.py',here.parent/'blackhole_q38_bf16_activation_start.S',here.parent/'blackhole_q38_bf16_activation.ld'}
for compiler,source,flags in ((cxx,unit,()),(tool+'g++',here.parent/'blackhole_q38_bf16_activation.cpp',
 ('-march=rv32imf','-mabi=ilp32','-DPIONEER_Q38_BF16_ACTIVATION_IMAGE=1'))):
 data=subprocess.check_output([compiler,'-std=c++17',*flags,'-MM',str(source)],text=True)
 dependencies.update(Path(name).resolve() for name in data.replace('\\\n','').split(':',1)[1].split())
assert all(path.is_relative_to(root) for path in dependencies)
frozen={path:hashlib.sha256(path.read_bytes()).hexdigest() for path in dependencies};exits={};commands=[]
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False)
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_ACTIVATION_OFFBOARD',name,result.returncode,flush=True)
run('build',[sys.executable,here.parent/'build_q38_bf16_activation.py',sdk,out/'image'])
run('compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',unit,'-o',out/'producer-test'])
run('producer',[out/'producer-test',out/'image'])
run('fraction',[sys.executable,here/'pioneer_q38_cached_bf16_fraction_test.py',out/'producer.stdout'])
assert all(hashlib.sha256(path.read_bytes()).hexdigest()==digest for path,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_BF16_ACTIVATION_COMPLETE_OFFBOARD children'+str(len(exits))+'_stderr0_frozenSources'+str(len(frozen))+
 '_NO_TRANSPORT_WORKER_ROOT_NATIVE_HARDWARE_FULL_MODEL_OR_TPS_AUTHORITY',flush=True)
