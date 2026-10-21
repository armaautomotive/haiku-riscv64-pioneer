#!/usr/bin/env python3
"""Changed-rounding group32 requires its complete isolated native qualifier."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_banks_driver.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_Q4_GROUP32':'1','PIONEER_Q38_GROUP32_QUALIFICATION':'1',
 'PIONEER_Q38_BLOCK_SCALE_REUSE':'1','PIONEER_Q38_SCALE_REUSE_QUALIFICATION':'1',
 'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1','PIONEER_Q38_PREPARED_Q4_AFFINE':'0','PIONEER_Q38_PREDECODED_Q4':'0',
 'PIONEER_Q38_PANEL_DRIVER':'10','PIONEER_DRIVER_RELOAD':'1','PIONEER_Q38_INLINE_CONFIG':'1',
 'PIONEER_Q38_WORK_DEFER_AFFINE':'1','PIONEER_Q38_WORK_COMPACT_OUTPUT':'1','PIONEER_Q38_NORMAL_CLOCK':'1',
 'PIONEER_Q38_DISPATCH_PROFILE':'0','PIONEER_Q38_DISPATCH_BATCHED_PIO':'0','PIONEER_Q38_GDN_MUTABLE_STATE':'0',
 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC':'0','PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC':'0',
 'PIONEER_Q38_LOCAL_DELTA_GGML':'0','PIONEER_Q38_NATIVE_OPERAND_STORE':'1','PIONEER_Q38_WORK_MODE':'1'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','-1','invalid'):
  if wrong!=correct:cases.append({**required,key:wrong})
for value in ('1','-1','invalid','01',''):
 for scale in ('0','1'):
  for root in ('0','1'):
   cases.append({'PIONEER_Q38_Q4_GROUP32':value,'PIONEER_Q38_BLOCK_SCALE_REUSE':scale,
    'PIONEER_Q38_SCALE_REUSE_QUALIFICATION':'1','PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1',
    'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_ROOT_CYCLE_PROFILE':root})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-group32-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  result=subprocess.run(['sh',str(script),str(sdk),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} group32 incomplete-qualifier/mixed-role/layout refusals BEFORE SDK/output;NO native access')
