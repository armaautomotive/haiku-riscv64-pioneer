#!/usr/bin/env python3
"""No native/model entry point may inherit this unqualified root flag."""
import ast,os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_WORK_ROOT_COMMAND_REUSE':'1','PIONEER_Q38_WORK_SENTINEL_POLL':'1',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE':'1','PIONEER_Q38_WORK_COHORT_CACHE':'1','PIONEER_Q38_WORK_COMPACT_OUTPUT':'1',
 'PIONEER_Q38_WORK_BATCH4_FETCH':'0','PIONEER_Q38_FAULT_RETURN':'0',
 'PIONEER_Q38_TRANSFER_DIAGNOSTIC':'0','PIONEER_Q38_PEER_PROGRESS':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid'):
  if wrong==correct or (key=='PIONEER_Q38_WORK_ROOT_COMMAND_REUSE' and wrong=='0'):continue
  cases.append(('build_qwen38_dispatch_image.sh',{**required,key:wrong}))
for script in ('build_qwen38_banks_driver.sh','build_qwen38_banks_client.sh','build_qwen38_inference.sh','build_qwen38_persistent_rows_image.sh'):
 for value in ('1','01','-1','invalid'):cases.append((script,{'PIONEER_Q38_WORK_ROOT_COMMAND_REUSE':value}))
tree=ast.parse((directory/'tests/pioneer_q38_direct_srcb_build_gate_test.py').read_text())
driver=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='driver' for t in n.targets))
driver.update({'PIONEER_Q38_COMMAND_REUSE_QUALIFICATION':'1','PIONEER_Q38_WORK_ROOT_COMMAND_REUSE':'1',
 'PIONEER_Q38_SENTINEL_POLL_QUALIFICATION':'1','PIONEER_Q38_WORK_SENTINEL_POLL':'1',
 'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION':'0','PIONEER_Q38_WORK_TILE_PREFETCH':'0',
 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION':'0','PIONEER_Q38_WORK_RAW_UNPACK_CACHE':'0',
 'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION':'0','PIONEER_Q38_WORK_FRONTEND_BARRIER':'0',
 'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION':'0','PIONEER_Q38_WORK_BATCH4_FETCH':'0','PIONEER_Q38_WORK_BATCH_RAW_INPUT':'0',
 'PIONEER_Q38_HOST_PHASE_PROFILE':'0','PIONEER_Q38_DISPATCH_PROFILE':'0',
 'PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':'2026102100010024'})
client={'PIONEER_Q38_COMMAND_REFERENCE_CLIENT':'1','PIONEER_Q38_SENTINEL_REFERENCE_CLIENT':'1',
 'PIONEER_Q38_PREFETCH_REFERENCE_CLIENT':'0','PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_FRONTEND_REFERENCE_CLIENT':'0','PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_BATCH4_REFERENCE_CLIENT':'0','PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_PANEL_CLIENT':'4','PIONEER_Q38_VECTOR_CLIENT':'1','PIONEER_Q38_WORK_CLIENT':'1',
 'PIONEER_Q38_ROOT_CYCLE_CLIENT':'1','PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_GROUP32_REFERENCE_CLIENT':'0','PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT':'0',
 'PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT':'0'}
for script,flags,primary in [('build_qwen38_banks_driver.sh',driver,'PIONEER_Q38_COMMAND_REUSE_QUALIFICATION'),('build_qwen38_banks_client.sh',client,'PIONEER_Q38_COMMAND_REFERENCE_CLIENT')]:
 for key,correct in flags.items():
  for wrong in ('0','1','01','-1','invalid','2026102100010023'):
   if wrong==correct or (key==primary and wrong=='0'):continue
   cases.append((script,{**flags,key:wrong}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-command-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,out=root/'MUST_NOT_OPEN_SDK',root/('MUST_NOT_CREATE_'+str(i));args=[str(sdk),str(out)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(root/'MUST_NOT_OPEN_LIBRARIES'),str(out)]
  r=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not out.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} command-write reuse mixed-role/driver/client/peer/model/Assistant refusals BEFORE SDK/libraries/output;NO native access')
