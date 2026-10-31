#!/usr/bin/env python3
"""Authenticate new immutable program DATA; no native/card execution authority."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=[(root,helpers['pins'](here.parent/'build_qwen38_token_fault_marker_image.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_precise_beta_worker_image.py','PINS'))]
for directory,pins in inputs:helpers['authenticated'](directory,pins)
committed=here.parent/'PioneerQ38TokenPreciseBetaPrograms.h'
selected=''.join('#define '+role+' 1\n' for role in
                 ('PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'))
include='#include "PioneerQ38TokenSelectedPrograms.h"\n'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-beta-programs-',dir='/private/tmp') as name:
    out=pathlib.Path(name);header=out/'programs.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--precise-beta'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent),
            '-DPIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS=1','-DPIONEER_Q38_TOKEN_FAULT_PROGRAMS=1','-DPIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1']
    binary=out/'data-test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined','-DPIONEER_Q38_PROGRAM_DATA_TEST_PRECISE_BETA=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    refused=0
    for directory,pins in inputs:
        for artifact,_,_ in pins:
            case=out/f'bad-{refused}';case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/artifact;target.unlink() # private symlink only, never the actual input
            original=(directory/artifact).read_bytes();corrupt=bytearray(original);corrupt[len(corrupt)//2]^=1
            for data in (corrupt,original[:-1]):
                target.write_bytes(data);output=out/f'NO_OUTPUT-{refused}'
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--precise-beta'],capture_output=True)
                assert result.returncode==2 and artifact.encode() in result.stderr and not output.exists();refused+=1
    assert refused==30
    compiler=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent),'-fsyntax-only','-x','c++','-']
    good=selected+include+include+'static_assert(PioneerQ38TokenPrograms::PreciseBeta==1);\n'
    subprocess.run(compiler,input=good,text=True,check=True)
    bad=[]
    for role in ('PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'):
        bad += [good.replace('#define '+role+' 1\n',''),good.replace(role+' 1',role+' 0')]
    bad += ['#include "PioneerQ38TokenFaultPrograms.h"\n'+good,
            '#include "PioneerQ38TokenPushedPrograms.h"\n'+good,
            include+good,selected+include+'#undef PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS\n'+include]
    for role in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
                 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_VALIDATION_NATIVE',
                 'PIONEER_Q38_TOKEN_VALIDATION_CLIENT','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT',
                 'PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
        bad += ['#define '+role+' '+value+'\n'+good for value in ('0','1')]
    for source in bad:
        result=subprocess.run(compiler,input=source,text=True,capture_output=True)
        assert result.returncode!=0 and 'error:' in result.stderr
    # ROM DATA is compiled separately from the tensor-test TU. Do not make
    # its no-Work boundary permissive just because the old ISA fixture has
    # historical tensor-format flags named Work.
    bridge=here/'pioneer_q38_precise_beta_program_data_bridge.cpp';bridge_refused=0
    for role in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
                 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in ('0','1'):
            result=subprocess.run([compiler[0],'-std=c++17','-E','-D'+role+'='+value,str(bridge)],capture_output=True,text=True)
            assert result.returncode!=0 and 'Expected ROM DATA bridge is HOST-only' in result.stderr;bridge_refused+=1
    print('PASS precise-beta DATA/all15 authenticated artifacts/ALL120 immutable codewords/'+str(refused)+
          ' corrupt+truncated refusals/'+str(len(bad))+' foreign,defined-zero,old-owner and late selector refusals/'+
          str(bridge_refused)+' DATA-bridge foreign-role refusals;NO_NATIVE_PHYSICAL_WORK_OR_TPS')
