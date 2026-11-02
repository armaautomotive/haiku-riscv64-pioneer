#!/usr/bin/env python3
"""Explicit malformed producer CLI/environment refusals before SDK/output."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
builder=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_q4_constant_issue_worker_image.py'
with tempfile.TemporaryDirectory(prefix='pioneer-q4-constant-build-gates-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp)
    sdk=root/'ABSENT_SDK';out=root/'MUST_NOT_EXIST'
    cases=0
    for arguments in ([],[str(sdk)],['--unknown',str(sdk),str(out)],
                      ['--inspect-candidate','--inspect-candidate',str(sdk),str(out)],
                      [str(sdk),'relative-output']):
        result=subprocess.run([sys.executable,str(builder),*arguments],capture_output=True,text=True)
        assert result.returncode and not out.exists()
        cases+=1
    for role in ('PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE','PIONEER_Q38_Q4_CONSTANT_ISSUE',
                 'PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING','PIONEER_DRIVER_BUILD_TAG',
                 'PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        for value in ('','0','1','2','-1'):
            result=subprocess.run([sys.executable,str(builder),str(sdk),str(out)],capture_output=True,
                                  text=True,env={**os.environ,role:value})
            assert result.returncode and not out.exists() and 'Traceback' in result.stderr
            cases+=1
    print('PASS_CONSTANT_Q4_BUILDER_PRESDK_REFUSALS',cases,'NO output or SDK/device invocation')
