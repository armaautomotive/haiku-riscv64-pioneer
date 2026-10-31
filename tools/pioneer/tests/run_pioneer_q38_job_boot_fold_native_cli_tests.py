#!/usr/bin/env python3
"""Bounded full-END TEST selector refusals before compile/output;no device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
refused=0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-fold-native-cli-',dir='/private/tmp') as name:
    root=pathlib.Path(name);sentinel=root/'sentinel';sentinel.write_bytes(b'unchanged\n')
    paths=[str(root/'zero'),'--execution-end',str(root/'model'),str(root/'old'),str(root/'worker'),str(root/'prefix.bin')]
    flags=['--pushed-context','--precise-beta','--posted-cancel','--root-frontend-snapshots',
           '--held-hidden-readout','--job-boot-fold']
    cases=[[value for value in flags if value!=missing] for missing in flags if missing!='--job-boot-fold']
    cases += [['--job-boot-fold'],flags+['--job-boot-fold'],flags+['--unknown-job-boot-fold']]
    cases += [flags+[extra] for extra in ('--fault-marker','--precise-context','--backend-finish',
              '--control-adapter','--control-operations','--execution-fault-marker','--bootstrap-peer-engines')]
    # A supplied old ROOT profile map may not silently observe the new ROM.
    cases += [flags+['--root-instruction-profile',str(root)]]
    for options in cases:
        before=sorted(p.relative_to(root) for p in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*paths,*options],capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr,(options,result.stderr)
        assert before==sorted(p.relative_to(root) for p in root.rglob('*')) and sentinel.read_bytes()==b'unchanged\n'
        refused+=1
    for mode in ('--execution-first-layer','--execution-full-flow','--execution-end-full-flow',
                 '--execution-end-state-full-flow','--execution-end-reload','--execution-cancel-first-layer',
                 '--execution-fault-marker','--bootstrap-peer-engines','--backend-initialization-faults'):
        changed=paths.copy();changed[1]=mode;before=sorted(p.relative_to(root) for p in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*changed,*flags],capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr,(mode,result.stderr)
        assert before==sorted(p.relative_to(root) for p in root.rglob('*'));refused+=1
    # Remove the new selector without forbidding the old legal profile:
    # the actual DATA tests separately prove it cannot claim JobBootFold.
    # Ambient overrides are refused before zero-image read or output. Use
    # this known file ONLY for the runner's pre-existing is_file check.
    paths[2]=str(here/'run_pioneer_q38_job_boot_fold_native_cli_tests.py')
    for key in ('PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT',
                'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        before=sorted(p.relative_to(root) for p in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*paths,*flags],env={**os.environ,key:'0'},capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr and 'FileNotFoundError' not in result.stderr
        assert before==sorted(p.relative_to(root) for p in root.rglob('*'));refused+=1
print(f'PASS_JOB_BOOT_FOLD_NATIVE_CLI {refused} missing+duplicate+foreign+shortcut+oldMap+ambientRoleRefusals unchangedOutputInventory1;NO compiler/native/model/card/Work/TPS grant')
