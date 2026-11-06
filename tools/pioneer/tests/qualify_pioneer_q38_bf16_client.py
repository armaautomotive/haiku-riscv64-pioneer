#!/usr/bin/env python3
"""Qualify BF16 USER pairing and transport lifecycle;no card execution."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

args=sys.argv[1:];profile='--profile' in args
if profile:assert args.count('--profile')==1;args.remove('--profile')
assert __debug__ and len(args)==3 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,libraries,out=map(Path,args);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,libraries,out)) and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);exits={};commands=[]
cxx=os.environ.get('CXX','c++')
common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-ffp-contract=off','-fsanitize=address,undefined']
flags=['-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1','-DPIONEER_Q38_BF16_FULL_CLIENT=1']
if profile:flags += ['-DPIONEER_Q38_BF16_FULL_PROFILE=1','-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1','-DPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE=1']
dependencies={Path(__file__).resolve(),here.parent/'build_assistant_qwen27_bf16_full_offload.py',
 here.parent/'build_assistant_qwen27_full_offload.py',here.parent/'pioneer_assistant_qwen27_bf16_full_offload.cpp',
 here.parent/'pioneer_assistant_qwen27_full_offload.cpp'}
if profile:dependencies.add(here.parent/'pioneer_assistant_qwen27_bf16_profile_full_offload.cpp')
for name in ('protocol','session'):
 source=here/('pioneer_q38_full_offload_'+name+'_test.cpp')
 text=subprocess.check_output([cxx,'-std=c++17',*flags,'-MM',str(source)],text=True)
 dependencies.update(Path(p).resolve() for p in text.replace('\\\n','').split(':',1)[1].split())
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_CLIENT',name,result.returncode,flush=True)
for name in ('protocol','session'):
 source=here/('pioneer_q38_full_offload_'+name+'_test.cpp');binary=out/name
 run(name+'-compile',[*common,*flags,source,'-o',binary]);run(name,[binary])
run('sdk-build',[sys.executable,here.parent/'build_assistant_qwen27_bf16_full_offload.py',sdk,libraries,out/'sdk',*(['--profile'] if profile else [])])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_BF16_USER_PAIRING children5_stderr0_sources'+str(len(frozen))+
 '_SDK_VOCABULARY_ONLY_AND_SYNTHETIC_TRANSPORT_NOT_NATIVE_EXECUTION_MODEL_QUALITY_OR_TPS',flush=True)
