#!/usr/bin/env python3
"""Build complete BF16 native image and exercise its journal OFFBOARD only."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
sdk,original,private,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,original,private,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);exits={};commands=[]
cxx=os.environ.get('CXX','c++')
dependencies={Path(__file__).resolve(),here/'qualify_pioneer_q38_bf16_root.py',here/'run_pioneer_q38_bf16_native_role_tests.py',
 here/'run_pioneer_q38_token_memory_tests.py',here.parent/'generate_qwen38_bf16_programs.py',
 here.parent/'generate_qwen38_token_programs.py',here.parent/'PioneerQ38Bf16ProgramPins.py',here.parent/'build_q38_bf16_full_native.py'}
legacyFlags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
for name,flags in (('pioneer_q38_token_memory_test.cpp',legacyFlags),('pioneer_q38_token_footprint_test.cpp',
 ['-DPIONEER_Q38_TOKEN_PROGRAMS_HEADER="../PioneerQ38TokenSelectedPrograms.h"'])):
 data=subprocess.check_output([cxx,'-std=c++17','-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1',*flags,'-MM',str(here/name)],text=True)
 dependencies.update(Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split())
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}

def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False)
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_NATIVE_OFFBOARD',name,result.returncode,flush=True)

run('root-and-worker',[sys.executable,here/'qualify_pioneer_q38_bf16_root.py',sdk,original,private,out/'root-and-worker'])
image=out/'root-and-worker/root';worker=out/'root-and-worker/worker-suite/worker'
run('generate',[sys.executable,here.parent/'generate_qwen38_bf16_programs.py',image,worker,out/'programs.h'])
assert (out/'programs.h').read_bytes()==(here.parent/'PioneerQ38TokenBf16Programs.h').read_bytes()
run('native-build',[sys.executable,here.parent/'build_q38_bf16_full_native.py',sdk,out/'native'])
run('roles',[sys.executable,here/'run_pioneer_q38_bf16_native_role_tests.py'])
run('footprint-compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
 '-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_TOKEN_PROGRAMS_HEADER="../PioneerQ38TokenSelectedPrograms.h"',
 here/'pioneer_q38_token_footprint_test.cpp','-o',out/'footprint-test'])
run('footprint',[out/'footprint-test',image,worker,'1'])
for name,arg in (('journal-positive','--positive'),('journal-faults','--faults'),('journal-capsule','--capsule-faults')):
 run(name,[sys.executable,here/'run_pioneer_q38_token_memory_tests.py','--bf16-programs',arg])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
for manifest in (out/'root-and-worker/source-sha256.txt',out/'native/source-sha256.txt'):
 for line in manifest.read_text().splitlines():
  digest,name=line.split('  ',1);path=root/name
  assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
  if path in frozen:assert frozen[path]==digest
  frozen[path]=digest
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
artifacts=json.loads((out/'root-and-worker/artifact-sha256.json').read_text())
for name in ('programs.h','native/native.o','native/tenstorrent_pci_probe-bf16','native/unchanged-native/tenstorrent_pci_probe-default'):
 path=out/name;artifacts[name]=[path.stat().st_size,hashlib.sha256(path.read_bytes()).hexdigest()]
(out/'artifact-sha256.json').write_text(json.dumps(artifacts,sort_keys=True,indent=2)+'\n')
print('PASS_COMPLETE_BF16_NATIVE_BUILD_JOURNAL_OFFBOARD children'+str(len(exits))+'_stderr0_sources'+str(len(frozen))+
 '_NOT_WHOLE_ROOT_EXECUTION_NATIVE_RUNNING_LIFECYCLE_PHYSICAL_MODEL_QUALITY_OR_TPS_PERMISSION',flush=True)
