#!/usr/bin/env python3
"""Coalesced peer reads cannot enter root,driver,model or Assistant roles."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cases=[]
for script in ('build_qwen38_banks_driver.sh','build_qwen38_inference.sh','build_qwen38_dispatch_image.sh'):
 for value in ('1','01','-1','invalid'):
  cases.append((script,{'PIONEER_Q38_WORK_BATCH4_FETCH':value}))
required={'PIONEER_Q38_WORK_BATCH4_FETCH':'1','PIONEER_Q38_WORK_DIRECT_SRCB':'1',
 'PIONEER_Q38_WORK_AFFINE_REPLAY':'1','PIONEER_Q38_WORK_DOT_ZERO_ELISION':'1'}
for key,correct in required.items():
 for wrong in ('0','01','-1','invalid'):
  if key=='PIONEER_Q38_WORK_BATCH4_FETCH' and wrong=='0':continue
  cases.append(('build_qwen38_persistent_rows_image.sh',{**required,key:wrong}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-batch4-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  args=[str(sdk),str(output)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(root/'MUST_NOT_OPEN_LIBRARIES'),str(output)]
  r=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} batch4 peer-only/root/driver/model/Assistant refusals BEFORE SDK/output')
