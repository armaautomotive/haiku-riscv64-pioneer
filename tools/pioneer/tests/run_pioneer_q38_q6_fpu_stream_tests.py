#!/usr/bin/env python3
"""OFFBOARD complete Q6 GDDR/SFPU/FPU full-K image,NOT driver/hardware admission."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
sdk,original,output=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,original,output)) and sdk.is_dir() and original.is_file()
assert not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2]
names=('PioneerQ38Q6FpuBlockData.h','PioneerQ38Q6FpuStreamLayout.h','PioneerQ38Q6FpuStreamLoader.h',
       'PioneerQ38Q6FpuStreamCompute.h','blackhole_q38_q6_fpu_stream.cpp','blackhole_q38_q6_fpu_stream_start.S',
       'blackhole_q38_q6_fpu_stream.ld','build_q38_q6_fpu_stream.py','PioneerModelWeightCache.cpp',
       'build_q38_q4_fpu_stream.py','blackhole_q38_q4_fpu_stream.cpp',
       'blackhole_q38_q4_fpu_stream_start.S','blackhole_q38_q4_fpu_stream.ld',
       'build_q38_q4_fpu_block.py','blackhole_q38_q4_fpu_block.cpp',
       'blackhole_q38_q4_fpu_block_start.S','blackhole_q38_q4_fpu_block.ld')
tests=('pioneer_q38_q6_fpu_data_test.cpp','pioneer_q38_q6_fpu_pipeline_test.cpp',
       'pioneer_q38_q6_fpu_pipeline_machine.h','pioneer_q38_q6_fpu_stream_loader_test.cpp',
       'pioneer_q38_q6_fpu_stream_pipeline_test.cpp','pioneer_q38_q6_fpu_stream_image_test.cpp',
       'pioneer_q38_q6_fpu_stream_fault_test.cpp','pioneer_q38_q6_fpu_stream_model_test.cpp',
       'pioneer_q38_q6_fpu_stream_fraction_test.py','run_pioneer_q38_q6_fpu_stream_tests.py',
       'pioneer_native_bfp_arithmetic_test.py','run_pioneer_q38_q4_fpu_block_tests.sh',
       'pioneer_q38_q4_fpu_block_data_test.cpp','pioneer_q38_q4_fpu_pipeline_test.cpp',
       'pioneer_q38_q4_fpu_block_image_test.cpp','pioneer_q38_q4_fpu_pipeline_fraction_test.py',
       'run_pioneer_q38_mixed_fpu_decode_tests.sh','pioneer_q38_mixed_fpu_decode_test.cpp',
       'pioneer_q38_mixed_fpu_fraction_test.py')
pending=[here.parent/n for n in names]+[here/n for n in tests];pins={}
while pending:
    path=pending.pop().resolve();name=str(path.relative_to(root))
    if name in pins:continue
    data=path.read_bytes();pins[name]=hashlib.sha256(data).hexdigest()
    if path.suffix in ('.h','.cpp','.inc'):
        for include in re.findall(rb'^\s*#\s*include\s*"([^"\n]+)"',data,re.M):pending.append(path.parent/include.decode('ascii'))
output.mkdir(mode=0o700);commands=[]
env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
def run(label,command):
    commands.append(command);closed=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    (output/(label+'.log')).write_bytes(closed.stdout);(output/(label+'.stderr')).write_bytes(closed.stderr)
    (output/(label+'.exit')).write_text(str(closed.returncode)+'\n')
    if closed.returncode or closed.stderr:raise RuntimeError(f'{label} CLOSED{closed.returncode} stderr_bytes{len(closed.stderr)};inspect evidence,NO promotion')
    print('PASS_CLOSED',label,'exit0 stdout_sha'+hashlib.sha256(closed.stdout).hexdigest(),'stderr0',flush=True)
images={5120:'af5b7c2e7ea4d1fa28ba7e0b11f8122bc5168ce8ba1416be1ef5b1319135b867',
        6144:'f6ced20d6861124fa23134dfe5af9fa70d7eb8ba8cf679b0fa101af242c3934d'}
for k,digest in images.items():
    run('build'+str(k),[sys.executable,str(here.parent/'build_q38_q6_fpu_stream.py'),str(sdk),str(output/('image'+str(k))),str(k),'--size'])
    data=(output/('image'+str(k))/'text.bin').read_bytes();assert len(data)==7848 and hashlib.sha256(data).hexdigest()==digest
cxx=os.environ.get('CXX','c++');common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
sources={'data':'pioneer_q38_q6_fpu_data_test.cpp','block':'pioneer_q38_q6_fpu_pipeline_test.cpp',
         'loader':'pioneer_q38_q6_fpu_stream_loader_test.cpp','pipeline':'pioneer_q38_q6_fpu_stream_pipeline_test.cpp',
         'image':'pioneer_q38_q6_fpu_stream_image_test.cpp','fault':'pioneer_q38_q6_fpu_stream_fault_test.cpp',
         'model':'pioneer_q38_q6_fpu_stream_model_test.cpp'}
for name,file in sources.items():
    extra=[str(here.parent/'PioneerModelWeightCache.cpp')] if name=='model' else []
    run(name+'-compile',common+[str(here/file),*extra,'-o',str(output/name)])
for name in ('data','block','loader','pipeline'):run(name,[str(output/name)])
for k in images:run('image'+str(k),[str(output/'image'),str(output/('image'+str(k))),str(k)])
run('fault',[str(output/'fault'),str(output/'image5120')])
run('fraction',[sys.executable,str(here/'pioneer_q38_q6_fpu_stream_fraction_test.py'),str(output/'image5120.log'),str(output/'image6144.log')])
run('model',[str(output/'model'),str(output/'image5120'),str(output/'image6144'),str(original)])
run('old-q4-regression',['sh',str(here/'run_pioneer_q38_q4_fpu_block_tests.sh'),str(sdk)])
run('mixed-regression',['sh',str(here/'run_pioneer_q38_mixed_fpu_decode_tests.sh')])
for k,digest in ((5120,'7e4765f9ec803cc5986534ec4f947b93e5a32b4711a01223e5f39741dfce8e03'),
                 (17408,'cc22a0cb005ff037d751c0b307cc74c76586260e1910789616cbc0719d912295')):
    run('old-q4-build'+str(k),[sys.executable,str(here.parent/'build_q38_q4_fpu_stream.py'),str(sdk),str(output/('old-q4-image'+str(k))),str(k),'--size'])
    data=(output/('old-q4-image'+str(k))/'text.bin').read_bytes();assert len(data)==7824 and hashlib.sha256(data).hexdigest()==digest
gates=0
for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
    refused=subprocess.run([cxx,'-std=c++17','-D'+role,'-fsyntax-only',str(here/sources['loader'])],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert refused.returncode and not refused.stdout and b'Q6 streaming prototype is not a driver/Work/reference capability' in refused.stderr;gates+=1
for role in ('_KERNEL_MODE','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
    refused=subprocess.run([cxx,'-std=c++17','-D'+role,'-fsyntax-only',str(here/sources['data'])],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert refused.returncode and not refused.stdout and b'Q6 FPU data is a separate CPU preparation recipe only' in refused.stderr;gates+=1
for path,digest in pins.items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest
(output/'source-sha256.txt').write_text(''.join(pins[p]+'  '+p+'\n' for p in sorted(pins)))
(output/'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(f'PASS OFFBOARD full-K Q6 stream:{len(commands)} strict CLOSED0/stderr0 children,two7848B images,actual unchanged RV32/NoC/physical L1+Dst/FENCE/persistent4phase FPU,32 failures+7 numeric controls,independent FULL-K Fraction,6 authentic model bands/384words,unchanged old Q4 images/regression/mixed decoder,{gates} compile refusals,{len(pins)} complete source hashes;NOT native owner/hardware/model-quality/real-token/speed authority')
