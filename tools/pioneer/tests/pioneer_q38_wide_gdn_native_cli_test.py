#!/usr/bin/env python3
"""New wide native TEST CLI refuses incomplete/old lifetimes before IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable,str(here/'pioneer_q38_batched_output_closure_native_cli_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-wide-gdn-native-cli-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp);a=root/'root';b=root/'worker';a.mkdir();b.mkdir();output=root/'MUST_NOT_EXIST.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old',str(b),str(output)]
    selectors=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots',
               '--held-hidden-readout','--job-boot-fold','--token-frontier','--retained-conv-closure',
               '--commit-boundary-seals','--cohort-local-sources','--packet-identity-boundary','--tid-boundary']
    tail=['--flat-matrix-root',str(a),'--prefetch-worker',str(b),'--twin-batch-prefetch',
          '--frontier-failure-detail','--frontier-failure-link','--batched-output-closure','--wide-gdn']
    cases=[common+[s for s in selectors if s!=flag]+tail for flag in selectors]
    for flag in ('--twin-batch-prefetch','--frontier-failure-detail','--frontier-failure-link','--batched-output-closure'):
        cases.append(common+selectors+[s for s in tail if s!=flag])
    for mode in ('--execution-first-layer','--execution-end-reload','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--bootstrap-peer-engines','--preparation-endpoint-contract'):
        changed=common[:];changed[1]=mode;cases.append(changed+selectors+tail)
    cases += [common+selectors+tail+[flag] for flag in ('--wide-gdn','--wide-gdn=0','--compile-link-only','--batched-prefetch')]
    before=sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))
    for args in cases:
        result=subprocess.run([sys.executable,str(runner),*args],capture_output=True)
        assert result.returncode and b'AssertionError' in result.stderr and not result.stdout,(args,result.stderr)
        assert not output.exists() and sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))==before
    print(f'PASS wide-GDN native CLI/{len(cases)} missing,duplicate,shortened,old,compile-only,Work aliases refused BEFORE source/compiler/output IO;'
          'ALL unchanged corrected-output+QFL1 CLI+const-ROM bridge FIRST;NOT native lifetime,model or hardware proof')
    cpp=here/'pioneer_q38_wide_gdn_program_data_bridge.cpp';binary=root/'wide-bridge-test'
    flags=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
    subprocess.run([*flags,'-DPIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_WIDE_GDN=1',str(cpp),
                    str(here/'pioneer_q38_twin_batch_prefetch_program_data_bridge_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    refused=0
    for key in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                'PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE',
                'PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE','PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST',
                'PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE','PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST',
                'PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST','PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC'):
        for value in ('0','1'):
            result=subprocess.run([*flags,'-D'+key+'='+value,'-fsyntax-only',str(cpp)],capture_output=True)
            assert result.returncode and b'error:' in result.stderr;refused+=1
    print(f'PASS actual wide-GDN const-ROM bridge/all119 peer maps+unchanged output on refusal/{refused} CPU,Work,native,shader+arithmetic-TEST isolation refusals;NO model or hardware proof')
