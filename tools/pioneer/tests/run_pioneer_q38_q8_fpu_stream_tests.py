#!/usr/bin/env python3
"""OFFBOARD complete Q8 GDDR/SFPU/FPU full-K image,NOT driver/hardware admission."""
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
names=('PioneerQ38Q8FpuBlockData.h','PioneerQ38Q8FpuStreamLayout.h','PioneerQ38Q8FpuStreamLoader.h',
       'PioneerQ38Q8FpuStreamCompute.h','blackhole_q38_q8_fpu_stream.cpp','blackhole_q38_q8_fpu_stream_start.S',
       'blackhole_q38_q8_fpu_stream.ld','build_q38_q8_fpu_stream.py','PioneerModelWeightCache.cpp',
       'build_q38_q4_fpu_stream.py','blackhole_q38_q4_fpu_stream.cpp',
       'blackhole_q38_q4_fpu_stream_start.S','blackhole_q38_q4_fpu_stream.ld',
       'build_q38_q4_fpu_block.py','blackhole_q38_q4_fpu_block.cpp',
       'blackhole_q38_q4_fpu_block_start.S','blackhole_q38_q4_fpu_block.ld')
tests=('pioneer_q38_q8_fpu_data_test.cpp','pioneer_q38_q8_fpu_pipeline_test.cpp',
       'pioneer_q38_q8_fpu_pipeline_machine.h','pioneer_q38_q8_fpu_stream_loader_test.cpp',
       'pioneer_q38_q8_fpu_stream_pipeline_test.cpp','pioneer_q38_q8_fpu_stream_image_test.cpp',
       'pioneer_q38_q8_fpu_stream_fault_test.cpp','pioneer_q38_q8_fpu_stream_model_test.cpp',
       'pioneer_q38_q8_fpu_stream_fraction_test.py','run_pioneer_q38_q8_fpu_stream_tests.py',
       'pioneer_native_bfp_arithmetic_test.py','run_pioneer_q38_q4_fpu_block_tests.sh',
       'pioneer_q38_q4_fpu_block_data_test.cpp','pioneer_q38_q4_fpu_pipeline_test.cpp',
       'pioneer_q38_q4_fpu_block_image_test.cpp','pioneer_q38_q4_fpu_pipeline_fraction_test.py',
       'run_pioneer_q38_mixed_fpu_decode_tests.sh','pioneer_q38_mixed_fpu_decode_test.cpp',
       'pioneer_q38_mixed_fpu_fraction_test.py')
pending=[here.parent/n for n in names]+[here/n for n in tests];pins={}
old_manifest=root/'tools/pioneer/benchmarks/qwen38-q6-fpu-stream-offboard-20261104/main/source-sha256.txt'
assert hashlib.sha256(old_manifest.read_bytes()).hexdigest()=='47ce6bc991c48c54e5bb16f436d649f082f6d208bc427ba9d7c80d0aeddd74e2'
pending.append(old_manifest)
for line in old_manifest.read_text().splitlines():
    digest,path=line.split('  ');assert re.fullmatch('[0-9a-f]{64}',digest)
    source=(root/path).resolve();source.relative_to(root)
    assert hashlib.sha256(source.read_bytes()).hexdigest()==digest;pending.append(source)
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
images={5120:'8ab2edda60d0b59f09d3f8162a04a10d2eec6a3aefe9a48f69188d4979c2f66e',
        6144:'5948d8b1aad1292d5c589449822f0d63c0dcf357c10c77916973d588568bf99d'}
for k,digest in images.items():
    run('build'+str(k),[sys.executable,str(here.parent/'build_q38_q8_fpu_stream.py'),str(sdk),str(output/('image'+str(k))),str(k),'--size'])
    data=(output/('image'+str(k))/'text.bin').read_bytes();assert len(data)==6812 and hashlib.sha256(data).hexdigest()==digest
cxx=os.environ.get('CXX','c++');common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
sources={'data':'pioneer_q38_q8_fpu_data_test.cpp','block':'pioneer_q38_q8_fpu_pipeline_test.cpp',
         'loader':'pioneer_q38_q8_fpu_stream_loader_test.cpp','pipeline':'pioneer_q38_q8_fpu_stream_pipeline_test.cpp',
         'image':'pioneer_q38_q8_fpu_stream_image_test.cpp','fault':'pioneer_q38_q8_fpu_stream_fault_test.cpp',
         'model':'pioneer_q38_q8_fpu_stream_model_test.cpp'}
for name,file in sources.items():
    extra=[str(here.parent/'PioneerModelWeightCache.cpp')] if name=='model' else []
    run(name+'-compile',common+[str(here/file),*extra,'-o',str(output/name)])
for name in ('data','block','loader','pipeline'):run(name,[str(output/name)])
for k in images:run('image'+str(k),[str(output/'image'),str(output/('image'+str(k))),str(k)])
run('fault',[str(output/'fault'),str(output/'image5120')])
run('fraction',[sys.executable,str(here/'pioneer_q38_q8_fpu_stream_fraction_test.py'),str(output/'image5120.log'),str(output/'image6144.log')])
run('model',[str(output/'model'),str(output/'image5120'),str(output/'image6144'),str(original)])
run('old-q6-regression',[sys.executable,str(here/'run_pioneer_q38_q6_fpu_stream_tests.py'),str(sdk),str(original),str(output/'old-q6-regression')])
gates=0
for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
    refused=subprocess.run([cxx,'-std=c++17','-D'+role,'-fsyntax-only',str(here/sources['loader'])],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert refused.returncode and not refused.stdout and b'Q8 streaming prototype is not a driver/Work/reference capability' in refused.stderr;gates+=1
for role in ('_KERNEL_MODE','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
    refused=subprocess.run([cxx,'-std=c++17','-D'+role,'-fsyntax-only',str(here/sources['data'])],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert refused.returncode and not refused.stdout and b'Q8 FPU data is a separate CPU preparation recipe only' in refused.stderr;gates+=1
for path,digest in pins.items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest
(output/'source-sha256.txt').write_text(''.join(pins[p]+'  '+p+'\n' for p in sorted(pins)))
(output/'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(f'PASS OFFBOARD full-K Q8 stream:{len(commands)} strict CLOSED0/stderr0 children,two6812B images,actual unchanged RV32/NoC/physical L1+Dst/FENCE/persistent4phase FPU,32 failures+8 numeric controls,independent FULL-K Fraction,11 authentic model bands/704words incl16-row tail,unchanged old Q6/Q4 images/regressions/mixed decoder,{gates} compile refusals,{len(pins)} complete source hashes;NOT native owner/hardware/model-quality/real-token/speed authority')
