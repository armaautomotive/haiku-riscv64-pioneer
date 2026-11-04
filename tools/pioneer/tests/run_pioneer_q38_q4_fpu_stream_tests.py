#!/usr/bin/env python3
"""OFFBOARD full-K Q4 stream:actual RV32/NoC/real-source/fault/Fraction checks,NO GPU."""
import hashlib
import os
import pathlib
import subprocess
import sys
assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
sdk,original,output=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,original,output)) and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2]
paths=['tools/pioneer/'+p for p in ('PioneerQ38Q4FpuStreamCompute.h','PioneerQ38Q4FpuStreamLayout.h',
    'PioneerQ38Q4FpuStreamLoader.h','blackhole_q38_q4_fpu_stream.cpp','blackhole_q38_q4_fpu_stream_start.S',
    'blackhole_q38_q4_fpu_stream.ld','build_q38_q4_fpu_stream.py')]
paths+=['tools/pioneer/tests/'+p for p in ('pioneer_q38_q4_fpu_stream_loader_test.cpp','pioneer_q38_q4_fpu_stream_pipeline_test.cpp',
    'pioneer_q38_q4_fpu_stream_image_test.cpp','pioneer_q38_q4_fpu_stream_fault_test.cpp','pioneer_q38_q4_fpu_stream_model_test.cpp',
    'pioneer_q38_q4_fpu_stream_fraction_test.py','run_pioneer_q38_q4_fpu_stream_tests.py','pioneer_q38_q4_fpu_pipeline_machine.h')]
pins={path:hashlib.sha256((root/path).read_bytes()).hexdigest() for path in paths}
output.mkdir(mode=0o700);commands=[]
def run(label,command):
    commands.append(command);r=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (output/(label+'.log')).write_bytes(r.stdout);(output/(label+'.stderr')).write_bytes(r.stderr)
    if r.returncode or r.stderr:raise RuntimeError(f'{label} CLOSED{r.returncode} stderr_bytes{len(r.stderr)};inspect PRIVATE evidence before reporting')
    print('PASS_CLOSED '+label+' exit0 stdout_sha'+hashlib.sha256(r.stdout).hexdigest()+' stderr0',flush=True)
for k in (5120,17408):run('build'+str(k),[sys.executable,str(here.parent/'build_q38_q4_fpu_stream.py'),str(sdk),str(output/('image'+str(k))),str(k),'--size'])
cxx=os.environ.get('CXX','c++');common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
for name in ('loader','pipeline','image','fault','model'):
    extra=[str(here.parent/'PioneerModelWeightCache.cpp')] if name=='model' else []
    run(name+'-compile',common+[str(here/('pioneer_q38_q4_fpu_stream_'+name+'_test.cpp')),*extra,'-o',str(output/name)])
for name in ('loader','pipeline'):run(name,[str(output/name)])
for k in (5120,17408):run('image'+str(k),[str(output/'image'),str(output/('image'+str(k))),str(k)])
run('fault',[str(output/'fault'),str(output/'image5120')])
run('fraction',[sys.executable,str(here/'pioneer_q38_q4_fpu_stream_fraction_test.py'),str(output/'image5120.log'),str(output/'image17408.log')])
run('model',[str(output/'model'),str(output/'image5120'),str(output/'image17408'),str(original)])
run('old-regression',['sh',str(here/'run_pioneer_q38_q4_fpu_block_tests.sh'),str(sdk)])
for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION'):
    r=subprocess.run([cxx,'-std=c++17','-D'+role,'-fsyntax-only',str(here/'pioneer_q38_q4_fpu_stream_loader_test.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert r.returncode and b'Q4 streaming prototype is not a driver/Work/reference capability' in r.stderr
for path,digest in pins.items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest
(output/'source-sha256.txt').write_text('\n'.join(digest+'  '+path for path,digest in pins.items())+'\n')
(output/'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print('PASS OFFBOARD full-K Q4 stream:two7824B images,actual strict RV32/NoC/canonical L1/FENCE/persistent FPU+failure controls+independent FULL-K Fraction+9 authentic model bands/576 words+unchanged old7036B image/regression+3 role refusals;NOT native owner/hardware/model-quality/real-token/speed authority')
