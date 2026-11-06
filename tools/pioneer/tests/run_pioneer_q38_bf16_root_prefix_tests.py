#!/usr/bin/env python3
"""Execute actual cold ROOT+Q8/GDN prefix to first BF16 packet OFFBOARD."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
image,old,worker,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (image,old,worker,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sys.path.insert(0,str(here.parent))
from generate_qwen38_token_programs import authenticated,pins
from PioneerQ38Bf16ProgramPins import ROOT_PINS,WORKER_PINS
authenticated(image,ROOT_PINS);authenticated(worker,WORKER_PINS)
authenticated(old,pins(here/'run_pioneer_q38_mixed_worker_tests.py','PINS'))
out.mkdir(mode=0o700);exits={};commands=[]
cxx=os.environ.get('CXX','c++')
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
dependencies={Path(__file__).resolve(),here/'run_pioneer_q38_ffn_persistent_image_tests.sh',
 here/'run_pioneer_q38_mixed_worker_tests.py',here.parent/'PioneerQ38Bf16ProgramPins.py',here.parent/'generate_qwen38_token_programs.py'}
for name,extra in (('pioneer_q38_bf16_root_prefix_test.cpp',[*flags,'-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1']),
                   ('pioneer_q38_paired_projection_probe.cpp',[])):
 data=subprocess.check_output([cxx,'-std=c++17',*extra,'-MM',str(here/name)],text=True)
 dependencies.update(Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split())
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_ACTUAL_BF16_ROOT_PREFIX',name,result.returncode,flush=True)
run('probe-compile',[*common,'-c',here/'pioneer_q38_paired_projection_probe.cpp','-o',out/'probe.o'])
run('prefix-compile',[*common,*flags,'-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1',here/'pioneer_q38_bf16_root_prefix_test.cpp',out/'probe.o','-o',out/'prefix-test'])
run('prefix',[out/'prefix-test',image,old,worker])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_ACTUAL_COLD_BF16_ROOT_PREFIX children3_stderr0_source'+str(len(frozen))+
 '_GENUINE_PREFIX_ONLY_NOT_BF16_PACKET_COMPLETION_WHOLE_LAYER_NATIVE_PHYSICAL_MODEL_QUALITY_OR_TPS',flush=True)
