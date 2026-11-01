#!/usr/bin/env python3
"""Reject incomplete prefetch native histories BEFORE source/compiler IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable,str(here/'pioneer_q38_root_frontend_cli_role_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-prefetch-native-cli-',dir='/private/tmp') as temporary:
    base=pathlib.Path(temporary);root=base/'root';worker=base/'worker';other=base/'other'
    for p in (root,worker,other):p.mkdir()
    prefix=base/'MUST_NOT_EXIST.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old',str(worker),str(prefix)]
    selectors=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots','--held-hidden-readout',
               '--job-boot-fold','--token-frontier','--retained-conv-closure','--commit-boundary-seals',
               '--cohort-local-sources','--packet-identity-boundary','--tid-boundary']
    tail=['--flat-matrix-root',str(root),'--prefetch-worker',str(worker)]
    cases=[]
    for flag in selectors:cases.append(common+[v for v in selectors if v!=flag]+tail)
    for mode in ('--execution-first-layer','--execution-end-reload','--execution-full-flow','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--bootstrap-peer-engines','--execution-readout-empty'):
        changed=common[:];changed[1]=mode;cases.append(changed+selectors+tail)
    changed=common[:];changed[4]=str(other);cases.append(changed+selectors+tail)
    cases += [common+selectors+tail+['--prefetch-worker',str(worker)],
              common+selectors+['--prefetch-worker',str(worker)],
              common+selectors+tail+['--compile-link-only'],
              common+selectors+tail+['--root-instruction-profile',str(root)]]
    before=sorted(p.relative_to(base).as_posix() for p in base.rglob('*'))
    for args in cases:
        result=subprocess.run([sys.executable,str(runner),*args],capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr and not result.stdout,(args,result.stderr)
        assert not prefix.exists() and sorted(p.relative_to(base).as_posix() for p in base.rglob('*'))==before
    print(f'PASS prefetch native CLI/{len(cases)} missing+duplicate+wrongworker+shortened+profile+compile-only refusals/'
          'NO source,compiler,runtime or output creation;NOT native execution or device qualification')
