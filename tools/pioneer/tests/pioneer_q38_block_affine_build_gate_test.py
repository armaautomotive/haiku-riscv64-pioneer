#!/usr/bin/env python3
"""Initial native cache qualifier requires a separate complete timing chain."""
import os
import pathlib
import subprocess
import tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_banks_driver.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1','PIONEER_Q38_PREPARED_Q4_AFFINE':'0',
 'PIONEER_Q38_PANEL_DRIVER':'10','PIONEER_DRIVER_RELOAD':'1','PIONEER_Q38_INLINE_CONFIG':'1',
 'PIONEER_Q38_WORK_DEFER_AFFINE':'1','PIONEER_Q38_WORK_COMPACT_OUTPUT':'1','PIONEER_Q38_NORMAL_CLOCK':'1',
 'PIONEER_Q38_DISPATCH_PROFILE':'0','PIONEER_Q38_DISPATCH_BATCHED_PIO':'0','PIONEER_Q38_GDN_MUTABLE_STATE':'0',
 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC':'0','PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC':'0',
 'PIONEER_Q38_LOCAL_DELTA_GGML':'0','PIONEER_Q38_NATIVE_OPERAND_STORE':'1','PIONEER_Q38_WORK_MODE':'1'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','-1','invalid'):
  if wrong!=correct: cases.append({**required,key:wrong})
cases += [{'PIONEER_Q38_BLOCK_AFFINE_DST':'1'}, {'PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1'}]
with tempfile.TemporaryDirectory(prefix='pioneer-q38-block-affine-build-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(script),str(sdk),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(flags,r.returncode,r.stderr)
  assert not output.exists() and not sdk.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} block-affine dependency/ordinary-layout/mixed-role refusals BEFORE SDK/output;NO native access')
