#!/usr/bin/env python3
"""Attribute unchanged cold ROOT instructions to ELF symbols OFFBOARD."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
sdk,image,old,worker,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,image,old,worker,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sys.path.insert(0,str(here.parent))
from generate_qwen38_token_programs import authenticated,pins
from PioneerQ38Bf16ProgramPins import ROOT_PINS,WORKER_PINS
authenticated(image,ROOT_PINS);authenticated(worker,WORKER_PINS)
authenticated(old,pins(here/'run_pioneer_q38_mixed_worker_tests.py','PINS'))
out.mkdir(mode=0o700);exits={};commands=[]
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump')
symbols={}
for row in subprocess.check_output([tool,'-t','-C',str(image/'program.elf')],text=True).splitlines():
 p=row.split(maxsplit=5)
 if len(p)==6 and p[2]=='F' and int(p[4],16):
  a,n=int(p[0],16),int(p[4],16)
  assert (0x20000<=a and a+n<=0x30000) or (0x40000<=a and a+n<=0x60000) or (0x64000<=a and a+n<=0x6c000)
  if a in symbols:assert symbols[a][0]==n
  else:symbols[a]=(n,p[5])
assert len(symbols)>=200
mapping=out/'profile-map.txt';mapping.write_text(''.join(f'{a:x} {n:x} {name}\n' for a,(n,name) in sorted(symbols.items())))
mapHash=hashlib.sha256(mapping.read_bytes()).hexdigest()
cxx=os.environ.get('CXX','c++');source=here/'pioneer_q38_bf16_prefix_profile_test.cpp'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
flags+=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1']
data=subprocess.check_output([cxx,'-std=c++17',*flags,'-MM',str(source)],text=True)
dependencies={Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split()}
dependencies.update((Path(__file__).resolve(),here/'run_pioneer_q38_ffn_persistent_image_tests.sh',
 here/'run_pioneer_q38_mixed_worker_tests.py',here.parent/'generate_qwen38_token_programs.py',here.parent/'PioneerQ38Bf16ProgramPins.py'))
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1',
    'PIONEER_TEST_ROOT_PROFILE_MAP':str(mapping)})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_PREFIX_PROFILE',name,result.returncode,flush=True)
run('compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
 '-fsanitize=address,undefined',*flags,source,'-o',out/'profile-test'])
run('profile',[out/'profile-test',image,old,worker])
assert hashlib.sha256(mapping.read_bytes()).hexdigest()==mapHash
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_ACTUAL_COLD_PREFIX_PROFILE children2_stderr0_sources'+str(len(frozen))+
 '_SYMBOL_ATTRIBUTION_ONLY_NOT_FULL_MODEL_OR_PHYSICAL_CYCLES_FREQUENCY_TPS',flush=True)
