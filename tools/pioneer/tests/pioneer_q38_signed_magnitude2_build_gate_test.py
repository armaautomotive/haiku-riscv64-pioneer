#!/usr/bin/env python3
"""Separate OFFLINE signed decoder; reject native/mixed roles before SDK IO."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
flag='PIONEER_Q38_WORK_SIGNED_MAG2'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
role={flag:'1','PIONEER_Q38_VECTOR_PERSISTENT_IMAGE':'9',
      'PIONEER_Q38_WORK_DIRECT_SRCB':'1','PIONEER_Q38_WORK_AFFINE_REPLAY':'1',
      'PIONEER_Q38_NATIVE_OPERAND_STORE':'1','PIONEER_Q38_STATIC_VECTOR':'1',
      'PIONEER_Q38_REPLAY_OPERAND':'1'}
cases=[]
for script in ('build_qwen38_banks_driver.sh','build_qwen38_banks_client.sh','build_qwen38_inference.sh','build_qwen38_dispatch_image.sh'):
 for value in ('1','01','-1','invalid'):cases.append((script,{flag:value}))
image='build_qwen38_persistent_rows_image.sh'
for key in role:
 for value in ('0','01','-1','invalid'):
  if key!=flag or value!='0':cases.append((image,{**role,key:value}))
for key in ('PIONEER_Q38_WORK_PEER_PHASE_PROFILE','PIONEER_Q38_WORK_FRONTEND_BARRIER',
            'PIONEER_Q38_WORK_RAW_UNPACK_CACHE','PIONEER_Q38_WORK_BATCH4_FETCH',
            'PIONEER_Q38_WORK_BATCH_RAW_INPUT','PIONEER_Q38_WORK_TILE_PREFETCH','PIONEER_Q38_WORK_Q4_NIBBLE'):
 for value in ('1','01','-1','invalid'):cases.append((image,{**role,key:value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-signed-mag2-gates-') as name:
 root=pathlib.Path(name);empty=root/'EMPTY_PATH';empty.mkdir()
 for script,flags in cases:
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/'MUST_NOT_CREATE_OUTPUT'
  args=[str(sdk),str(out)] if script!='build_qwen38_inference.sh' else [str(sdk),str(libs),str(out)]
  r=subprocess.run(['/bin/sh',str(directory/script),*args],env={**base,**flags,'PATH':str(empty)},capture_output=True,text=True)
  assert r.returncode==2 and 'not found' not in r.stderr,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists()
print(f'PASS signed magnitude2 {len(cases)} native/mixed/partial/invalid role refusals before ANY external command/SDK/output;NO native selector')
