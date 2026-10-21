#!/usr/bin/env python3
"""Coalesced reads need a complete isolated native qualifier,never Assistant."""
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
driver={'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION':'1','PIONEER_Q38_WORK_BATCH4_FETCH':'1',
 'PIONEER_Q38_SENTINEL_POLL_QUALIFICATION':'1','PIONEER_Q38_HOST_PHASE_PROFILE':'0',
 'PIONEER_Q38_DISPATCH_PROFILE':'0','PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':'2026102100010019'}
client={'PIONEER_Q38_BATCH4_REFERENCE_CLIENT':'1','PIONEER_Q38_SENTINEL_REFERENCE_CLIENT':'1',
 'PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT':'0','PIONEER_Q38_PANEL_CLIENT':'4',
 'PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1','PIONEER_Q38_ROOT_CYCLE_CLIENT':'1',
 'PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0','PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0','PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'0'}
for script,flags,primary in [('build_qwen38_banks_driver.sh',driver,'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION'),('build_qwen38_banks_client.sh',client,'PIONEER_Q38_BATCH4_REFERENCE_CLIENT')]:
 for key,correct in flags.items():
  for wrong in ('0','1','01','-1','invalid','2026102100010018','2026102100010020'):
   if wrong==correct or (key==primary and wrong=='0'):continue
   cases.append((script,{**flags,key:wrong}))
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
