#!/usr/bin/env python3
"""QGW3 flags fail closed BEFORE model/image IO or output creation."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
profile=['--unroll-dense-seals','--boundary-seals','--unroll-quiet','--worker-local-source-guards',
         '--precise-activation','--single-closed-audit','--worker-sealed-admission','--pushed-completion']
mode='--pushed-completion-faults'
checks=0
with tempfile.TemporaryDirectory(prefix='pioneer-qgw3-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary)
    def reject(script,inputs,flags,environment=None):
        global checks
        result=subprocess.run([sys.executable,str(script),*[str(root/name) for name in inputs],*flags],
                              capture_output=True,text=True,timeout=30,env={**os.environ,**(environment or {})})
        assert result.returncode and 'AssertionError' in result.stderr,(script,flags,result.stdout,result.stderr)
        assert 'FileNotFoundError' not in result.stderr and not any(root.iterdir()),'must refuse before input IO/output creation'
        checks+=1
    for script,extra in (('build_qwen38_global_token_components.py',['--f32-hidden']),
                         ('build_qwen38_token_root.py',['--cancel-boundary','--f32-hidden'])):
        selected=[*extra,*profile]
        for flag in selected:
            if flag!='--pushed-completion':reject(here.parent/script,['sdk','output'],[f for f in selected if f!=flag])
        for flag in ('--pushed-completion','--pushed-completion=0','--unknown-qgw3'):
            reject(here.parent/script,['sdk','output'],[*selected,flag])
    runner=here/'run_pioneer_q38_token_original_flow_tests.py'
    for selected_mode,other_mode in ((mode,'--pushed-cohort-faults'),('--pushed-cohort-faults',mode)):
        selected=['--cancel-boundary',*profile,selected_mode]
        for flag in selected:
            if flag!=selected_mode:reject(runner,['image','mixed','worker','model.gguf'],[f for f in selected if f!=flag])
        for flag in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory',
                     '--snapshot-first-layer','--eos-after-prompt','--admission-source-faults',selected_mode,other_mode,'--pushed-completion=0'):
            reject(runner,['image','mixed','worker','model.gguf'],[*selected,flag])
        reject(runner,['image','mixed','worker','model.gguf'],[*selected,'--observed-trace',str(root/'observed')])
        for name in ('PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
            reject(runner,['image','mixed','worker','model.gguf'],selected,{name:'0'})
print(f'PASS QGW3 receiver CLI/{checks} missing-parent+shortcut+duplicate+unknown+defined-zero-environment refusals BEFORE model/image IO or output creation;NO runtime/native/physical/Work/TPS grant')
