#!/usr/bin/env python3
"""Check isolated linked layer-trace ROOT; no card or native authority."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 4 or not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_BASELINE ABS_TRACE_A ABS_TRACE_B')
baseline, first, second = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (baseline, first, second)):
    raise SystemExit('absolute build directories required')
def pin(path, digest):
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest: raise RuntimeError('hash mismatch: '+str(path))
pin(baseline/'program.elf','8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f')
for folder in (first,second):
    pin(folder/'program.elf','a103f6d5134355ba7d0af0479e7f09f34e31591850fa2fc566abfc079821e402')
    pin(folder/'conv.bin','9d952770b4107d2981657429351fb1e3224fdfb9d2cddb0b8271b3869fc06f58')
    for name in ('token-components-NOBOOT.o','unchanged-feedback/feedback.o'):
        if (folder/'qualified-components'/name).read_bytes() != (baseline/'qualified-components'/name).read_bytes():
            raise RuntimeError('component differs: '+name)
here=Path(__file__).resolve().parent; root=here.parent; cxx=os.environ.get('CXX','c++')
role='PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE'
names=('__riscv','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK',
       'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU','PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
       'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE',
       'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION',role)
flags=['-D'+n+'=1' for n in names]
include='#include "'+str(root/'PioneerQ38Qgw8LayerTraceRole.h')+'"\n'
def check(options, source, good):
    result=subprocess.run([cxx,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    if (result.returncode==0)!=good: raise RuntimeError(repr(options)+' '+result.stderr[:1000])
check(flags,include,True)
for flag in flags:
    check([f for f in flags if f!=flag],include,flag==flags[-1])
    check([f for f in flags if f!=flag]+[flag[:-1]+'0'],include,False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
             'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129','PIONEER_Q38_QGW8_HIDDEN_CLIENT_129',
             'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
             'PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION','PIONEER_Q38_HYBRID_FULL_NATIVE',
             'PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE',
             'PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
             'PIONEER_Q38_TOKEN_FRONTIER_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(flags+['-D'+name+'=1'],include,False)
check(flags[:-1],include+'#define '+role+' 1\n'+include,False)
check(flags,include+'#undef '+role+'\n'+include,False)
assembler_role='PIONEER_Q38_QGW8_TRACE_ASSEMBLER_INSPECTION'
assembler='#include "'+str(root/'PioneerQ38Qgw8LayerTraceAssembler.h')+'"\n'
check(['-D'+assembler_role+'=1'],assembler,True)
check([],assembler,False)
check(['-D'+assembler_role+'=0'],assembler,False)
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                  'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT',
                  'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
                  'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
                  'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128','PIONEER_Q38_QGW8_HIDDEN_NATIVE_129',
                  'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE',
                  'PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION','PIONEER_Q38_WORK_INFERENCE',
                  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(['-D'+assembler_role+'=1','-D'+forbidden+'=1'],assembler,False)
check(['-D'+assembler_role+'=1'],assembler+'#undef '+assembler_role+'\n'+assembler,False)
with tempfile.TemporaryDirectory(prefix='qgw8-layer-trace-test-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    geometry=Path(tmp)/'geometry'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_qgw8_layer_trace_geometry_test.cpp'),'-o',str(geometry)],check=True)
    subprocess.run([str(geometry)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    assembly=Path(tmp)/'assembly'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_qgw8_layer_trace_assembler_test.cpp'),'-o',str(assembly)],check=True)
    subprocess.run([str(assembly)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_qgw8_layer_trace_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(first)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    out=Path(tmp)/'must-not-exist.h'
    result=subprocess.run([sys.executable,str(root/'generate_qwen38_hybrid_programs.py'),'--qgw8-native-candidate',
                           str(first),str(Path(tmp)/'must-not-open-worker'),str(out)],capture_output=True,text=True)
    if not result.returncode or 'artifact text.bin' not in result.stderr or out.exists():
        raise RuntimeError('old-family generator did not reject trace image before worker open')
print('PASS isolated trace image, role refusals, pinned builds, old-family generator rejection; NO DEVICE OR WORK')
