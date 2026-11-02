#!/usr/bin/env python3
"""Fresh-slot reader TEST/producer early refusals;NO native/device execution."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable,str(here/'pioneer_q38_token_matrix_phase_native_cli_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-matrix-read-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);a,b=root/'root',root/'worker';a.mkdir();b.mkdir();output=root/'MUST_NOT_EXIST.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old',str(b),str(output)]
    selectors=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots',
               '--held-hidden-readout','--job-boot-fold','--token-frontier','--retained-conv-closure',
               '--commit-boundary-seals','--cohort-local-sources','--packet-identity-boundary','--tid-boundary']
    tail=['--flat-matrix-root',str(a),'--prefetch-worker',str(b),'--twin-batch-prefetch','--frontier-failure-detail',
          '--frontier-failure-link','--batched-output-closure','--wide-gdn','--token-phase-profile','--constant-q4-issue',
          '--token-matrix-phase','--matrix-phase-held-readout']
    cases=[common+[s for s in selectors if s!=flag]+tail for flag in selectors]
    for flag in ('--twin-batch-prefetch','--frontier-failure-detail','--frontier-failure-link','--batched-output-closure',
                 '--wide-gdn','--token-phase-profile','--constant-q4-issue','--token-matrix-phase'):
        cases.append(common+selectors+[s for s in tail if s!=flag])
    for mode in ('--execution-first-layer','--execution-end-reload','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--bootstrap-peer-engines','--preparation-endpoint-contract'):
        changed=common[:];changed[1]=mode;cases.append(changed+selectors+tail)
    cases += [common+selectors+tail+[flag] for flag in
              ('--matrix-phase-held-readout','--matrix-phase-held-readout=0','--compile-link-only','--batched-prefetch','--work','--cpu')]
    before=sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))
    for args in cases:
        result=subprocess.run([sys.executable,str(runner),*args],capture_output=True)
        assert result.returncode and b'AssertionError' in result.stderr and not result.stdout,(args,result.stderr)
        assert not output.exists() and sorted(p.relative_to(root).as_posix() for p in root.rglob('*'))==before
    builder=here.parent/'build_qwen38_token_matrix_phase_read_native.py';early=0
    for args in ([],['--unknown',str(output)],['--inspect-candidate','--inspect-candidate',str(output)],['relative-output']):
        result=subprocess.run([sys.executable,str(builder),*args],capture_output=True)
        assert result.returncode and not output.exists();early+=1
    for key in ('PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE','PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT',
                'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        for value in ('','0','1','2','-1'):
            result=subprocess.run([sys.executable,str(builder),str(output)],capture_output=True,env={**os.environ,key:value})
            assert result.returncode and not output.exists();early+=1
    print(f'PASS fresh-slot reader TEST CLI/{len(cases)} incomplete,duplicate,shortened,Work aliases+{early} producer refusals BEFORE source/compiler/output IO;ALL sampler+older controls FIRST;omitting optional reader legitimately retains old no-reader subject,NOT a readout grant',flush=True)
