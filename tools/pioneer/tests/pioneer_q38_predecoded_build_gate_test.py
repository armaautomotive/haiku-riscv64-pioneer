#!/usr/bin/env python3
"""No native reader or expanded-weight owner exists:refuse BEFORE SDK/output."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_banks_driver.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cases=[]
for value in ('1','01','-1','invalid'):
 for scale in ('0','1'):
  for group in ('0','1'):
   cases.append({'PIONEER_Q38_PREDECODED_Q4':value,'PIONEER_Q38_BLOCK_SCALE_REUSE':scale,
    'PIONEER_Q38_Q4_GROUP32':group,'PIONEER_Q38_SCALE_REUSE_QUALIFICATION':'1',
    'PIONEER_Q38_BLOCK_AFFINE_DST':'1','PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION':'1',
    'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1'})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-predecoded-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i))
  result=subprocess.run(['sh',str(script),str(sdk),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} predecoded native/layout/mixed-role refusals BEFORE SDK/output;NO native access')
