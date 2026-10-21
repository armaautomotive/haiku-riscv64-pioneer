#!/usr/bin/env python3
"""Direct-input UNPACK is an offline-only,original-TC Work experiment."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_WORK_DIRECT_SRCB':'1','PIONEER_Q38_BLOCK_SCALE_REUSE':'1',
 'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_INLINE_CONFIG':'1',
 'PIONEER_Q38_WORK_DEFER_AFFINE':'1','PIONEER_Q38_WORK_ZERO_MINIMUM':'1',
 'PIONEER_Q38_WORK_INPUT_SUM_CACHE':'1','PIONEER_Q38_WORK_SKIP_DOT_CHECK':'1',
 'PIONEER_Q38_VECTOR_PERSISTENT_IMAGE':'9','PIONEER_Q38_NATIVE_Q8':'0',
 'PIONEER_Q38_PREDECODED_Q4':'0','PIONEER_Q38_Q4_GROUP32':'0',
 'PIONEER_Q38_PREPARED_Q4_BLOCK':'0','PIONEER_Q38_PREPARED_Q4_AFFINE':'0'}
cases=[]
driver={'PIONEER_Q38_DIRECT_SRCB_QUALIFICATION':'1','PIONEER_Q38_WORK_DIRECT_SRCB':'1',
 'PIONEER_Q38_AFFINE_RAWDOT_QUALIFICATION':'1','PIONEER_Q38_WORK_AFFINE_REPLAY':'1',
 'PIONEER_Q38_WORK_DOT_ZERO_ELISION':'1','PIONEER_Q38_BLOCK_SCALE_REUSE':'1',
 'PIONEER_Q38_SCALE_REUSE_QUALIFICATION':'1','PIONEER_Q38_BLOCK_AFFINE_DST':'1',
 'PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1','PIONEER_Q38_ROOT_CYCLE_PROFILE':'1',
 'PIONEER_Q38_NATIVE_Q8':'0','PIONEER_Q38_NATIVE_Q8_QUALIFICATION':'0',
 'PIONEER_Q38_GROUP32_QUALIFICATION':'0','PIONEER_Q38_Q4_GROUP32':'0',
 'PIONEER_Q38_NATIVE_Q8_LAYOUT':'0','PIONEER_Q38_PREPARED_Q4_AFFINE':'0',
 'PIONEER_Q38_PREDECODED_Q4':'0','PIONEER_Q38_PREPARED_Q4_BLOCK':'0',
 'PIONEER_Q38_PREDECODED_Q4_STICKY':'0'}
client={'PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'1','PIONEER_Q38_PANEL_CLIENT':'4',
 'PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1','PIONEER_Q38_ROOT_CYCLE_CLIENT':'1',
 'PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0','PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0'}
for script,flags in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for key,correct in flags.items():
  for wrong in ('0','1','01','-1','invalid'):
   if wrong==correct or (key=='PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT' and wrong=='0'):continue
   cases.append((script,{**flags,key:wrong}))
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid'):
  if wrong==correct or (key=='PIONEER_Q38_WORK_DIRECT_SRCB' and wrong=='0'):continue
  cases.append(('build_qwen38_persistent_rows_image.sh',{**required,key:wrong}))
for value in ('1','01','-1','invalid'):
 for script in ('build_qwen38_banks_driver.sh','build_qwen38_inference.sh'):
  cases.append((script,{'PIONEER_Q38_WORK_DIRECT_SRCB':value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-direct-srcb-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  args=[str(sdk),str(output)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(root/'MUST_NOT_OPEN_LIBRARIES'),str(output)]
  result=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(script,flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} direct-SrcB offline/mixed-role/native/inference refusals BEFORE SDK/output;NO card access')
