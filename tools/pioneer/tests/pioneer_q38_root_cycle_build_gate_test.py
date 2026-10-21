#!/usr/bin/env python3
"""Refuse incomplete/mixed timing diagnostics before SDK or outputs."""
import os
import pathlib
import subprocess
import tempfile
script = pathlib.Path(__file__).resolve().parent.parent / 'build_qwen38_banks_driver.sh'
base = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
required = {'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1', 'PIONEER_Q38_PANEL_DRIVER':'10',
 'PIONEER_DRIVER_RELOAD':'1', 'PIONEER_Q38_INLINE_CONFIG':'1', 'PIONEER_Q38_WORK_DEFER_AFFINE':'1',
 'PIONEER_Q38_WORK_COMPACT_OUTPUT':'1', 'PIONEER_Q38_NORMAL_CLOCK':'1',
 'PIONEER_Q38_DISPATCH_PROFILE':'0', 'PIONEER_Q38_DISPATCH_BATCHED_PIO':'0',
 'PIONEER_Q38_GDN_MUTABLE_STATE':'0', 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC':'0',
 'PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC':'0', 'PIONEER_Q38_LOCAL_DELTA_GGML':'0',
 'PIONEER_Q38_NATIVE_OPERAND_STORE':'1', 'PIONEER_Q38_WORK_MODE':'1'}
cases = []
for key, correct in required.items():
 for wrong in ('0','1','-1','invalid'):
  if wrong != correct and not (key == 'PIONEER_Q38_ROOT_CYCLE_PROFILE' and wrong == '0'):
   cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-root-cycle-gate-') as directory:
 root = pathlib.Path(directory)
 for i, flags in enumerate(cases):
  sdk, output = root/'MUST_NOT_OPEN_SDK', root/('MUST_NOT_CREATE_'+str(i))
  result = subprocess.run(['sh',str(script),str(sdk),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode == 2, (flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} root-cycle dependency/mixed-role refusals BEFORE SDK/output;NO native operations')
