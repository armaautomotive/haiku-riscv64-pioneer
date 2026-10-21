#!/usr/bin/env python3
"""CPU in-memory phase profile has a separate diagnostic tag/complete chain."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_HOST_PHASE_PROFILE':'1','PIONEER_Q38_SENTINEL_POLL_QUALIFICATION':'1',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1','PIONEER_Q38_DISPATCH_PROFILE':'0','PIONEER_DRIVER_RELOAD':'1',
 'PIONEER_DRIVER_BUILD_TAG':'2026102100010018'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid','2026102100010017','2026102100010019'):
  if wrong!=correct and not(key=='PIONEER_Q38_HOST_PHASE_PROFILE' and wrong=='0'):
   cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-host-phase-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(directory/'build_qwen38_banks_driver.sh'),str(sdk),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(flags,r.returncode,r.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} host-phase wrong/missing role/tag/profile refusals BEFORE SDK/output')
client={'PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT':'1','PIONEER_Q38_SENTINEL_REFERENCE_CLIENT':'1',
 'PIONEER_Q38_PANEL_CLIENT':'4','PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1',
 'PIONEER_Q38_ROOT_CYCLE_CLIENT':'1','PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0','PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0','PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'0'}
count=0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-host-phase-client-gate-') as name:
 root=pathlib.Path(name)
 for key,correct in client.items():
  for wrong in ('0','1','01','-1','invalid'):
   if wrong==correct or (key=='PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT' and wrong=='0'):continue
   sdk,output=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_CREATE'
   r=subprocess.run(['sh',str(directory/'build_qwen38_banks_client.sh'),str(sdk),str(output)],env={**base,**client,key:wrong},capture_output=True,text=True)
   assert r.returncode==2,(key,wrong,r.returncode,r.stderr)
   assert not sdk.exists() and not output.exists() and str(sdk) not in r.stderr
   count+=1
print(f'PASS {count} host-phase diagnostic client role/format refusals BEFORE SDK/output')
