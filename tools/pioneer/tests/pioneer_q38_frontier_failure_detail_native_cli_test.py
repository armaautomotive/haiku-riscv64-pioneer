#!/usr/bin/env python3
"""Refuse shortened diagnostic histories before source/compiler/output IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable,str(here/'pioneer_q38_twin_batch_prefetch_native_cli_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-detail-native-cli-',dir='/private/tmp') as temporary:
    base=pathlib.Path(temporary);root,worker,other=(base/name for name in ('root','worker','other'))
    for directory in (root,worker,other):directory.mkdir()
    output=base/'MUST_NOT_EXIST.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old',str(worker),str(output)]
    selectors=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots',
               '--held-hidden-readout','--job-boot-fold','--token-frontier','--retained-conv-closure',
               '--commit-boundary-seals','--cohort-local-sources','--packet-identity-boundary','--tid-boundary']
    tail=['--flat-matrix-root',str(root),'--prefetch-worker',str(worker),'--twin-batch-prefetch','--frontier-failure-detail']
    cases=[common+[value for value in selectors if value!=flag]+tail for flag in selectors]
    for mode in ('--execution-first-layer','--execution-end-reload','--execution-full-flow','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--bootstrap-peer-engines','--execution-readout-empty',
                 '--preparation-endpoint-contract','--execution-backend-first-matrix'):
        changed=common[:];changed[1]=mode;cases.append(changed+selectors+tail)
    changed=common[:];changed[4]=str(other);cases.append(changed+selectors+tail)
    cases += [common+selectors+tail+['--frontier-failure-detail'],
              common+selectors+tail+['--compile-link-only'],
              common+selectors+tail+['--root-instruction-profile',str(root)],
              common+selectors+tail+['--batched-prefetch'],
              common+selectors+tail+['--twin-batch-prefetch'],
              common+selectors+tail+['--prefetch-worker',str(worker)],
              common+selectors+tail+['--counter-gather-root',str(root)],
              common+selectors+tail+['--empty-receipt-root',str(root)],
              common+selectors+['--frontier-failure-detail'],
              common+selectors+[value for value in tail if value!='--twin-batch-prefetch'],
              common+selectors+['--batched-prefetch','--frontier-failure-detail'],
              common+selectors+['--flat-matrix-root',str(root),'--frontier-failure-detail'],
              common+selectors+['--prefetch-worker',str(worker),'--frontier-failure-detail'],
              common+selectors+tail+['--frontier-failure-detail=0']]
    before=sorted(path.relative_to(base).as_posix() for path in base.rglob('*'))
    for args in cases:
        result=subprocess.run([sys.executable,str(runner),*args],capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr and not result.stdout,(args,result.stderr)
        assert not output.exists() and sorted(path.relative_to(base).as_posix() for path in base.rglob('*'))==before
    print(f'PASS failure-detail native CLI/{len(cases)} missing+duplicate+wrongworker+shortened+old+profile+'
          'compile-only+incompatible-ROOT refusals/ALL twin+ancestor controls FIRST;NO source,compiler,runtime or output creation;'
          'NOT native execution,model or hardware qualification')
