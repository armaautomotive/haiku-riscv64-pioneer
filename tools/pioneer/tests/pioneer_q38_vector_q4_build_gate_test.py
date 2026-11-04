#!/usr/bin/env python3
"""Malformed builder/environment and actual full shader-role refusals."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) in (1,2)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
builder = here.parent/'build_qwen38_vector_q4_metadata_worker.py'
with tempfile.TemporaryDirectory(prefix='pioneer-vector-q4-gates-',dir='/private/tmp') as tmp:
    root = pathlib.Path(tmp);out = root/'MUST_NOT_EXIST';sdk = root/'ABSENT_SDK';cases = 0
    for args in ([],[str(sdk)],['--unknown',str(sdk),str(out)],
                 ['--inspect-candidate','--inspect-candidate',str(sdk),str(out)],
                 [str(sdk),'relative-output']):
        result = subprocess.run([sys.executable,str(builder),*args],capture_output=True,text=True)
        assert result.returncode and not out.exists();cases += 1
    for role in ('PIONEER_Q38_VECTOR_Q4_METADATA','PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE',
                 'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        for value in ('','0','1','2','-1'):
            result = subprocess.run([sys.executable,str(builder),str(sdk),str(out)],capture_output=True,
                                    text=True,env={**os.environ,role:value})
            assert result.returncode and not out.exists();cases += 1
    print('PASS_VECTOR_Q4_BUILDER_PRESDK_REFUSALS',cases,'NO output,SDK or device invocation',flush=True)
    if len(sys.argv)==2:
        directory = pathlib.Path(sys.argv[1]);command = (directory/'compile-math-argv.txt').read_text().splitlines()
        command[command.index('-o')+1] = str(root/'MUST_NOT_COMPILE.o')
        command[command.index('-c')] = '-E'
        # The source TU itself must refuse late/missing image roles and all
        # CPU/native aliases,even when defined as zero. Never rely on a
        # runner refusing CLI flags to protect the actual compiled worker.
        refusals = [('-UPIONEER_Q38_VECTOR_Q4_METADATA',),('-UPIONEER_Q38_VECTOR_Q4_METADATA_IMAGE',),
                    ('-UPIONEER_Q38_VECTOR_Q4_METADATA','-DPIONEER_Q38_VECTOR_Q4_METADATA=0'),
                    ('-UPIONEER_Q38_VECTOR_Q4_METADATA_IMAGE','-DPIONEER_Q38_VECTOR_Q4_METADATA_IMAGE=0')]
        for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_CPU_VALIDATION',
                     'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
                     'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_Q4_CONSTANT_ISSUE',
                     'PIONEER_Q38_PREDECODED_Q4','PIONEER_Q38_NATIVE_Q8'):
            refusals.append(('-D'+role+'=0',))
        refusals.append(('-ffast-math',))
        for extra in refusals:
            result = subprocess.run([*command,*extra],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
            assert result.returncode and 'error:' in result.stderr,extra
        print('PASS_VECTOR_Q4_ACTUAL_SHADER_ROLE_REFUSALS',len(refusals),'NO execution/card IO',flush=True)
