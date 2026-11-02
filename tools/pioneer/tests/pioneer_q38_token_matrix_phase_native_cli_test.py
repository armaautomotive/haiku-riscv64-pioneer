#!/usr/bin/env python3
"""Separate sampler TEST CLI+const ROM bridge;NO native/device execution."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable,str(here/'pioneer_q38_token_constant_q4_native_cli_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-matrix-phase-native-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);a,b=root/'root',root/'worker';a.mkdir();b.mkdir();output=root/'MUST_NOT_EXIST.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old',str(b),str(output)]
    selectors=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots',
               '--held-hidden-readout','--job-boot-fold','--token-frontier','--retained-conv-closure',
               '--commit-boundary-seals','--cohort-local-sources','--packet-identity-boundary','--tid-boundary']
    tail=['--flat-matrix-root',str(a),'--prefetch-worker',str(b),'--twin-batch-prefetch','--frontier-failure-detail',
          '--frontier-failure-link','--batched-output-closure','--wide-gdn','--token-phase-profile','--constant-q4-issue','--token-matrix-phase']
    cases=[common+[s for s in selectors if s!=flag]+tail for flag in selectors]
    for flag in ('--twin-batch-prefetch','--frontier-failure-detail','--frontier-failure-link','--batched-output-closure',
                 '--wide-gdn','--token-phase-profile','--constant-q4-issue'):
        cases.append(common+selectors+[s for s in tail if s!=flag])
    for mode in ('--execution-first-layer','--execution-end-reload','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--bootstrap-peer-engines','--preparation-endpoint-contract'):
        changed=common[:];changed[1]=mode;cases.append(changed+selectors+tail)
    cases += [common+selectors+tail+[flag] for flag in
              ('--token-matrix-phase','--token-matrix-phase=0','--compile-link-only','--batched-prefetch','--work','--cpu')]
    before=sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))
    for args in cases:
        result=subprocess.run([sys.executable,str(runner),*args],capture_output=True)
        assert result.returncode and b'AssertionError' in result.stderr and not result.stdout,(args,result.stderr)
        assert not output.exists() and sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))==before
    print(f'PASS sampler native CLI/{len(cases)} missing,duplicate,shortened,old,compile-only refusals BEFORE source/compiler/output IO;ALL old constant-Q4+ancestors FIRST',flush=True)
    cpp=here/'pioneer_q38_token_matrix_phase_program_data_bridge.cpp';binary=root/'bridge-test'
    flags=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
    subprocess.run([*flags,'-DPIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_MATRIX_PHASE=1',str(cpp),
                    str(here/'pioneer_q38_twin_batch_prefetch_program_data_bridge_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    refused=0
    for key in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT',
                'PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE','PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT',
                'PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE','PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST',
                'PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE',
                'PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE'):
        for value in ('0','1'):
            result=subprocess.run([*flags,'-D'+key+'='+value,'-fsyntax-only',str(cpp)],capture_output=True)
            assert result.returncode and b'error:' in result.stderr;refused+=1
    builder=here.parent/'build_qwen38_token_matrix_phase_native.py';early=0
    for args in ([],['--unknown',str(output)],['--inspect-candidate','--inspect-candidate',str(output)],['relative-output']):
        result=subprocess.run([sys.executable,str(builder),*args],capture_output=True)
        assert result.returncode and not output.exists();early+=1
    for key in ('PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS','PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT',
                'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        for value in ('','0','1','2','-1'):
            result=subprocess.run([sys.executable,str(builder),str(output)],capture_output=True,env={**os.environ,key:value})
            assert result.returncode and not output.exists();early+=1
    print(f'PASS sampler const-ROM bridge ALL119 peers+unchanged refusals/{refused} CPU,Work,native,USER,shader exclusions/{early} producer early refusals;NO model,ISA,native lifecycle,held metric readout,hardware or TPS proof',flush=True)
