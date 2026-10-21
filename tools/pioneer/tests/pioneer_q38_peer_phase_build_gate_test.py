#!/usr/bin/env python3
"""Private timing peer has NO installed/kernel/model/Assistant selector."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_WORK_PEER_PHASE_PROFILE':'1','PIONEER_Q38_VECTOR_PERSISTENT_IMAGE':'9',
 'PIONEER_Q38_WORK_DIRECT_SRCB':'1','PIONEER_Q38_WORK_AFFINE_REPLAY':'1',
 'PIONEER_Q38_WORK_FRONTEND_BARRIER':'0','PIONEER_Q38_WORK_RAW_UNPACK_CACHE':'0',
 'PIONEER_Q38_WORK_BATCH4_FETCH':'0','PIONEER_Q38_WORK_TILE_PREFETCH':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid','8'):
  if wrong==correct or (key=='PIONEER_Q38_WORK_PEER_PHASE_PROFILE' and wrong=='0'):continue
  cases.append(('build_qwen38_persistent_rows_image.sh',{**required,key:wrong}))
for script in ('build_qwen38_banks_driver.sh','build_qwen38_banks_client.sh','build_qwen38_dispatch_image.sh','build_qwen38_inference.sh'):
 for value in ('1','01','-1','invalid'):cases.append((script,{'PIONEER_Q38_WORK_PEER_PHASE_PROFILE':value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-peer-phase-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/('MUST_NOT_CREATE_'+str(i));args=[str(sdk),str(out)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(libs),str(out)]
  r=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} peer timing role/format/driver/root/client/model/Assistant refusals BEFORE SDK/libraries/output')
