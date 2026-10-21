#!/usr/bin/env python3
"""Scheduling experiment cannot select an installed/native/Assistant image."""
import ast,os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_WORK_SENTINEL_POLL':'1','PIONEER_Q38_ROOT_CYCLE_PROFILE':'1',
 'PIONEER_Q38_WORK_COHORT_CACHE':'1','PIONEER_Q38_WORK_COMPACT_OUTPUT':'1',
 'PIONEER_Q38_FAULT_RETURN':'0','PIONEER_Q38_TRANSFER_DIAGNOSTIC':'0','PIONEER_Q38_PEER_PROGRESS':'0'}
cases=[]
tree=ast.parse((directory/'tests/pioneer_q38_direct_srcb_build_gate_test.py').read_text())
driver=next(ast.literal_eval(node.value) for node in tree.body if isinstance(node,ast.Assign)
 and any(isinstance(target,ast.Name) and target.id=='driver' for target in node.targets))
driver.update({'PIONEER_Q38_SENTINEL_POLL_QUALIFICATION':'1','PIONEER_Q38_WORK_SENTINEL_POLL':'1'})
client={'PIONEER_Q38_SENTINEL_REFERENCE_CLIENT':'1','PIONEER_Q38_PANEL_CLIENT':'4',
 'PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1','PIONEER_Q38_ROOT_CYCLE_CLIENT':'1',
 'PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0','PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0','PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'0'}
for script,flags in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for key,correct in flags.items():
  for wrong in ('0','1','01','-1','invalid'):
   if wrong==correct or (key=='PIONEER_Q38_SENTINEL_REFERENCE_CLIENT' and wrong=='0'):continue
   cases.append((script,{**flags,key:wrong}))
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid'):
  if wrong==correct or (key=='PIONEER_Q38_WORK_SENTINEL_POLL' and wrong=='0'):continue
  cases.append(('build_qwen38_dispatch_image.sh',{**required,key:wrong}))
for script in ('build_qwen38_banks_driver.sh','build_qwen38_inference.sh'):
 for value in ('1','01','-1','invalid'):cases.append((script,{'PIONEER_Q38_WORK_SENTINEL_POLL':value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-sentinel-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,output=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i));args=[str(sdk),str(output)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(root/'MUST_NOT_OPEN_LIBRARIES'),str(output)]
  result=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(script,flags,result.returncode,result.stderr)
  assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} sentinel scheduling native/Assistant/mixed-role refusals BEFORE SDK/output;NO device access')
