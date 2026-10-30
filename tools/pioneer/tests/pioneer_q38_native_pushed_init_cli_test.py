#!/usr/bin/env python3
"""Explicit G3 native TEST profile refusals BEFORE any image/model IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
runner=pathlib.Path(__file__).with_name('run_pioneer_q38_token_native_initialization_tests.py')
checks=0
with tempfile.TemporaryDirectory(prefix='pioneer-qtp3-init-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary)
    def reject(arguments,environment=None):
        global checks
        result=subprocess.run([sys.executable,str(runner),str(root/'zero'),*arguments],capture_output=True,text=True,
                              timeout=30,env={**os.environ,**(environment or {})})
        assert result.returncode and 'AssertionError' in result.stderr,(arguments,result.stdout,result.stderr)
        assert 'FileNotFoundError' not in result.stderr and not any(root.iterdir()),'refuse before input IO or output creation'
        checks+=1
    for mode in ('--faults','--late-faults','--held','--held-faults','--runtime','--runtime-pci-capture-fault',
                 '--runtime-faults','--bootstrap','--bootstrap-faults','--execution','--execution-faults',
                 '--execution-traffic','--execution-traffic-faults','--terminal-observation-faults',
                 '--preparation-fifo-faults','--preparation-access-faults','--initializer-fifo-faults','--clock-owner-contract'):
        reject([mode,'--pushed-context'])
    baseline=['--bootstrap-peer-engines',str(root/'mixed'),str(root/'worker'),'--pushed-context']
    for flag in ('--pushed-context','--precise-context','--pushed-context=0','--unknown-qtp3'):
        reject([*baseline,flag])
    reject(['--pushed-context'])
    for name in ('PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS','TT_PIONEER_Q38_WORK_MODE'):
        reject(baseline,{name:'0'})
print(f'PASS QTP3 native TEST CLI/{checks} synthetic-bootstrap+shortcut+duplicate+mixed+unknown+defined-zero-environment refusals before image/model IO;NO runtime/native/physical/Work/TPS grant')
