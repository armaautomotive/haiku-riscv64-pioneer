#!/usr/bin/env python3
"""Pinned BF16 timing observer qualification OFFBOARD; no native/card IO."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
sdk,image,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,image,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sys.path.insert(0,str(here.parent))
from PioneerQ38Bf16ProfileProgramPins import ROOT_PINS
for name,size,digest in ROOT_PINS:
 data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
out.mkdir(mode=0o700);exits={};commands=[]
cxx=os.environ.get('CXX','c++');tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
sources={Path(__file__).resolve(),here.parent/'PioneerQ38Bf16ProfileProgramPins.py',
 here/'run_pioneer_q38_bf16_profile_role_tests.py',here.parent/'PioneerQ38Bf16ProfileBuildRole.h',
 here.parent/'PioneerQ38Bf16ProfileScope.h',here.parent/'PioneerQ38TokenFrontierImageRole.h',
 here.parent/'PioneerQ38TokenPhaseProfileImageRole.h'}
for name,extra in (('pioneer_q38_bf16_profile_data_test.cpp',[]),
 ('pioneer_q38_token_phase_profile_image_test.cpp',['-DPIONEER_Q38_BF16_PROFILE_HELPER_TEST=1'])):
 dependencies=subprocess.check_output([cxx,'-std=c++17',*extra,'-MM',str(here/name)],text=True)
 sources.update(Path(p).resolve() for p in dependencies.replace('\\\n','').split(':',1)[1].split())
assert all(p.is_relative_to(root) for p in sources)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
def run(name,command):
 command=list(map(str,command));commands.append([name,*command])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(command,stdout=stdout,stderr=stderr,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,indent=2)+'\n')
 assert not result.returncode and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_TIMING',name,'exit0/stderr0',flush=True)
run('roles',[sys.executable,here/'run_pioneer_q38_bf16_profile_role_tests.py',sdk])
# Authenticate whole linked images and exact code-section snapshots, not an
# independently compiled helper or a host replacement for its instructions.
for section in ('boot','text','terminal','conv','bf16'):
 path=out/(section+'.bin')
 subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,image/'program.elf',path],check=True)
 assert path.read_bytes()==(image/(section+'.bin')).read_bytes(),section
assert subprocess.check_output([tool+'nm','-n',str(image/'program.elf')],text=True)==(image/'program.symbols').read_text()
run('data-compile',[cxx,*flags,here/'pioneer_q38_bf16_profile_data_test.cpp','-o',out/'data-test'])
run('data',[out/'data-test'])
run('helpers-compile',[cxx,*flags,'-DPIONEER_Q38_BF16_PROFILE_HELPER_TEST=1',
 here/'pioneer_q38_token_phase_profile_image_test.cpp','-o',out/'helper-test'])
run('helpers',[out/'helper-test',image])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_BF16_TIMING_OBSERVER children5_stderr0_sources'+str(len(frozen))+
 '_EXACT_LINKED_HELPERS_ONLY_SYNTHETIC_CLOCKS_NOT_FULL_ROOT_NATIVE_PHYSICAL_CLOCK_OR_TPS',flush=True)
