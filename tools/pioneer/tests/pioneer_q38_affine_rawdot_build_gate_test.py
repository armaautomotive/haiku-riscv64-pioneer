#!/usr/bin/env python3
"""Exact original-TC qualifier/client roles refuse unsafe mixtures before SDK IO."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
driver={
 'PIONEER_Q38_AFFINE_RAWDOT_QUALIFICATION':'1',
 'PIONEER_Q38_WORK_AFFINE_REPLAY':'1','PIONEER_Q38_WORK_DOT_ZERO_ELISION':'1',
 'PIONEER_Q38_BLOCK_SCALE_REUSE':'1','PIONEER_Q38_SCALE_REUSE_QUALIFICATION':'1',
 'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1','PIONEER_Q38_NATIVE_Q8':'0',
 'PIONEER_Q38_NATIVE_Q8_QUALIFICATION':'0','PIONEER_Q38_GROUP32_QUALIFICATION':'0',
 'PIONEER_Q38_Q4_GROUP32':'0','PIONEER_Q38_NATIVE_Q8_LAYOUT':'0',
 'PIONEER_Q38_PREPARED_Q4_AFFINE':'0','PIONEER_Q38_PREDECODED_Q4':'0',
 'PIONEER_Q38_PREPARED_Q4_BLOCK':'0','PIONEER_Q38_PREDECODED_Q4_STICKY':'0'}
client={'PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'1','PIONEER_Q38_PANEL_CLIENT':'4',
 'PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1',
 'PIONEER_Q38_ROOT_CYCLE_CLIENT':'1','PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0'}
cases=[]
for script,required in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for key,correct in required.items():
  for wrong in ('0','1','01','-1','invalid'):
   # The qualifier is optional only when ALL experimental flags are zero.
   if key=='PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT' and wrong=='0':continue
   if wrong!=correct:cases.append((script,{**required,key:wrong}))
for replay,dot in [('1','0'),('0','1'),('1','1'),('invalid','0'),('0','invalid')]:
 cases.append(('build_qwen38_banks_driver.sh',{'PIONEER_Q38_WORK_AFFINE_REPLAY':replay,'PIONEER_Q38_WORK_DOT_ZERO_ELISION':dot}))
for key in ('PIONEER_Q38_WORK_AFFINE_REPLAY','PIONEER_Q38_WORK_DOT_ZERO_ELISION'):
 for value in ('1','01','-1','invalid'):
  cases.append(('build_qwen38_inference.sh',{key:value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-affine-rawdot-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  args=[str(sdk),str(output)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(root/'MUST_NOT_OPEN_LIBRARIES'),str(output)]
  result=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(script,flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} affine/raw-dot driver/client/format/rounding/user-role refusals BEFORE SDK/output;NO native access')
