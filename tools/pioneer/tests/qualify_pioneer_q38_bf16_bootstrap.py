#!/usr/bin/env python3
"""Freeze and execute QTP4 cold bootstrap instructions OFFBOARD only."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==5
image,old,worker,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (image,old,worker,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
data=subprocess.check_output([os.environ.get('CXX','c++'),'-std=c++17',*flags,'-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1',
 '-MM',str(here/'pioneer_q38_token_bootstrap_test.cpp')],text=True)
dependencies={Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split()}
dependencies.update((Path(__file__).resolve(),here/'run_pioneer_q38_token_bootstrap_tests.py',
 here/'run_pioneer_q38_ffn_persistent_image_tests.sh',here.parent/'PioneerQ38Bf16ProgramPins.py'))
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
argv=[sys.executable,str(here/'run_pioneer_q38_token_bootstrap_tests.py'),str(image),str(old),str(worker),
 '--cancel-boundary','--f32-hidden','--bf16-fpu']
with (out/'bootstrap.stdout').open('xb') as stdout,(out/'bootstrap.stderr').open('xb') as stderr:
 result=subprocess.run(argv,stdout=stdout,stderr=stderr,check=False)
(out/'actual-exits.json').write_text(json.dumps({'bootstrap':result.returncode},indent=2)+'\n')
assert result.returncode==0 and not (out/'bootstrap.stderr').stat().st_size
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(argv,indent=2)+'\n')
print('PASS_ACTUAL_BF16_BOOTSTRAP child1_stderr0_sources'+str(len(frozen))+
 '_SAME119_VIRGIN_EMBEDDING_FIRST_LAYER_ENTRY_16_EMITTED_FAULTS_NOT_EXECUTED_WHOLE_LAYER_NATIVE_PHYSICAL_MODEL_QUALITY_OR_TPS',flush=True)
