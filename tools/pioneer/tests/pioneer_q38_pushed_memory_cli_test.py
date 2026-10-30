#!/usr/bin/env python3
"""G3 held-journal profile refusals BEFORE compile/output/SDK access."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
runner=pathlib.Path(__file__).with_name('run_pioneer_q38_token_memory_tests.py')
checks=0
with tempfile.TemporaryDirectory(prefix='pioneer-qtp3-journal-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary)
    def reject(arguments,environment=None):
        global checks
        result=subprocess.run([sys.executable,str(runner),*arguments],capture_output=True,text=True,timeout=30,
                              env={**os.environ,**(environment or {}),'TMPDIR':str(root)})
        assert result.returncode and 'AssertionError' in result.stderr,(arguments,result.stdout,result.stderr)
        assert 'CalledProcessError' not in result.stderr and 'FileNotFoundError' not in result.stderr and not any(root.iterdir())
        checks+=1
    base=['--pushed-programs']
    for flag in ('--pushed-programs','--precise-programs','--pushed-programs=0','--unknown-qtp3',
                 '--fast-root-memory','--pushed-context','--sdk'):
        reject([*base,flag])
    reject(['--capsule-faults'])
    reject(['--precise-programs','--precise-programs'])
    reject(['--precise-programs','--pushed-programs'])
    for name in ('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
        reject(base,{name:'0'})
print(f'PASS QTP3 held-journal CLI/{checks} duplicate+mixed+zero+unknown+shortcut+missing-profile/environment refusals BEFORE compile/IO;NO native,launch,Work or TPS grant')
