#!/usr/bin/env python3
"""OFFBOARD frozen85 client/parent/actual original/SDK qualification;NO GPU."""
import hashlib
import os
import pathlib
import subprocess
import sys
assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
sdk,private,original,output=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,private,original,output)) and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2]
paths=['tools/pioneer/PioneerNativeQ4FpuSource.h','tools/pioneer/pioneer_native_q4_fpu_diagnostic.cpp',
       'tools/pioneer/build_native_q4_fpu_client.py','tools/pioneer/pioneer_native_q4_fpu_physical.sh',
       'tools/pioneer/tests/pioneer_native_q4_fpu_client_test.py','tools/pioneer/tests/pioneer_native_q4_fpu_parent_test.py',
       'tools/pioneer/tests/pioneer_native_q4_fpu_output.py','tools/pioneer/tests/pioneer_native_q4_fpu_reference.py',
       'tools/pioneer/tests/pioneer_native_q4_fpu_source_probe.cpp','tools/pioneer/tests/run_pioneer_native_q4_fpu_client_tests.py',
       'tools/pioneer/benchmarks/qwen38-native-q4-fpu-reference-20261104.log']
pins={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}
output.mkdir(mode=0o700);commands=[]
def run(label,command):
    commands.append(command);result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (output/(label+'.log')).write_bytes(result.stdout);(output/(label+'.stderr')).write_bytes(result.stderr)
    if result.returncode or result.stderr:raise RuntimeError(f'{label} CLOSED{result.returncode} stderr_bytes{len(result.stderr)};inspect private outputs before reporting')
    print(f'PASS_CLOSED {label} exit0 stdout_sha'+hashlib.sha256(result.stdout).hexdigest()+' stderr0',flush=True)
    return result.stdout
run('sdk-client',[sys.executable,str(here.parent/'build_native_q4_fpu_client.py'),str(sdk),str(private),str(output/'build')])
run('parent-syntax',['sh','-n',str(here.parent/'pioneer_native_q4_fpu_physical.sh')])
run('client',[sys.executable,str(here/'pioneer_native_q4_fpu_client_test.py'),str(private)])
run('parent',[sys.executable,str(here/'pioneer_native_q4_fpu_parent_test.py')])
run('output',[sys.executable,str(here/'pioneer_native_q4_fpu_output.py'),'--self-test'])
reference=run('reference',[sys.executable,str(here/'pioneer_native_q4_fpu_reference.py'),str(private)])
assert reference==(root/paths[-1]).read_bytes()
run('source-compile',[os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off',
    '-I'+str(private),'-I'+str(here.parent),str(here/'pioneer_native_q4_fpu_source_probe.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(output/'source-probe')])
run('source',[str(output/'source-probe'),str(original)])
for path,digest in pins.items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest
(output/'source-sha256.txt').write_text('\n'.join(digest+'  '+path for path,digest in pins.items())+'\n')
(output/'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print('PASS OFFBOARD native85 client/parent SDK+1691 client+618 flow/output+617 independent child refusals+actual full original source;NO stage/device/native-engine/hardware/model-quality/speed authority')
