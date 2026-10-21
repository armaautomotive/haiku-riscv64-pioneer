#!/usr/bin/env python3
"""Prefetch peer remains OFFLINE and separate from native/model/root roles."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0;cases=[]
for script in ('build_qwen38_persistent_rows_image.sh','build_qwen38_banks_driver.sh','build_qwen38_dispatch_image.sh','build_qwen38_inference.sh'):
 for value in ('1','01','-1','invalid'):
  cases.append((script,{'PIONEER_Q38_WORK_TILE_PREFETCH':value}))
required={'PIONEER_Q38_WORK_TILE_PREFETCH':'1','PIONEER_Q38_WORK_DIRECT_SRCB':'1','PIONEER_Q38_WORK_AFFINE_REPLAY':'1',
 'PIONEER_Q38_VECTOR_PERSISTENT_IMAGE':'9','PIONEER_Q38_WORK_FRONTEND_BARRIER':'0','PIONEER_Q38_WORK_RAW_UNPACK_CACHE':'0',
 'PIONEER_Q38_WORK_BATCH4_FETCH':'0','PIONEER_Q38_WORK_BATCH_RAW_INPUT':'0'}
for key,correct in required.items():
 for value in ('0','1','01','-1','invalid','8'):
  if value==correct or (key=='PIONEER_Q38_WORK_TILE_PREFETCH' and value=='0'):continue
  cases.append(('build_qwen38_persistent_rows_image.sh',{**required,key:value}))
driver={'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION':'1','PIONEER_Q38_WORK_TILE_PREFETCH':'1',
 'PIONEER_Q38_SENTINEL_POLL_QUALIFICATION':'1','PIONEER_Q38_DIRECT_SRCB_QUALIFICATION':'1',
 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION':'0','PIONEER_Q38_WORK_RAW_UNPACK_CACHE':'0',
 'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION':'0','PIONEER_Q38_WORK_FRONTEND_BARRIER':'0',
 'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION':'0','PIONEER_Q38_WORK_BATCH4_FETCH':'0','PIONEER_Q38_WORK_BATCH_RAW_INPUT':'0',
 'PIONEER_Q38_HOST_PHASE_PROFILE':'0','PIONEER_Q38_DISPATCH_PROFILE':'0',
 'PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':'2026102100010023'}
client={'PIONEER_Q38_PREFETCH_REFERENCE_CLIENT':'1','PIONEER_Q38_SENTINEL_REFERENCE_CLIENT':'1',
 'PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT':'0','PIONEER_Q38_FRONTEND_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT':'0','PIONEER_Q38_BATCH4_REFERENCE_CLIENT':'0','PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_PANEL_CLIENT':'4','PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1','PIONEER_Q38_ROOT_CYCLE_CLIENT':'1',
 'PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0','PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0','PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'0'}
for script,flags,primary in [('build_qwen38_banks_driver.sh',driver,'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION'),('build_qwen38_banks_client.sh',client,'PIONEER_Q38_PREFETCH_REFERENCE_CLIENT')]:
 for key,correct in flags.items():
  for value in ('0','1','01','-1','invalid','2026102100010022','2026102100010024'):
   if value==correct or (key==primary and value=='0'):continue
   cases.append((script,{**flags,key:value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-prefetch-gate-') as name:
 root=pathlib.Path(name)
 for script,flags in cases:
   sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/('MUST_NOT_CREATE_'+str(count))
   args=[str(sdk),str(out)]
   if script=='build_qwen38_inference.sh':args=[str(sdk),str(libs),str(out)]
   result=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
   assert result.returncode==2 and str(sdk) not in result.stderr,(script,flags,result.returncode,result.stderr)
   assert not sdk.exists() and not libs.exists() and not out.exists();count+=1
print(f'PASS {count} prefetch peer/mixed-role/native/root/model refusals BEFORE SDK/libraries/output')
