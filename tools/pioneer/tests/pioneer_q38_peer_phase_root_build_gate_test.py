#!/usr/bin/env python3
"""Private timing-return transport has no native/model/Assistant selector."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={key:'1' for key in ('PIONEER_Q38_PEER_PHASE_ROOT','PIONEER_Q38_WORK_SENTINEL_POLL',
 'PIONEER_Q38_ROOT_CYCLE_PROFILE','PIONEER_Q38_WORK_COMPACT_OUTPUT','PIONEER_Q38_WORK_SPARSE_EXPORT',
 'PIONEER_Q38_OUTPUT_TRANSFER_ROOT','PIONEER_Q38_WORK_SKIP_DOT_CHECK','PIONEER_Q38_WORK_COHORT_CACHE',
 'PIONEER_Q38_REUSE_ACTIVATIONS','PIONEER_Q38_COMPACT_ACTIVATIONS')}
required.update({key:'0' for key in ('PIONEER_Q38_WORK_ROOT_COMMAND_REUSE','PIONEER_Q38_WORK_BATCH4_FETCH',
 'PIONEER_Q38_FAULT_RETURN','PIONEER_Q38_TRANSFER_DIAGNOSTIC','PIONEER_Q38_PEER_PROGRESS',
 'PIONEER_Q38_WORK_PEER_PHASE_PROFILE','PIONEER_Q38_WORK_TILE_PREFETCH','PIONEER_Q38_TILE_PREFETCH_QUALIFICATION',
 'PIONEER_Q38_WORK_RAW_UNPACK_CACHE','PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION',
 'PIONEER_Q38_WORK_FRONTEND_BARRIER','PIONEER_Q38_WORK_BATCH_RAW_INPUT','PIONEER_Q38_BATCH_RAW_QUALIFICATION')})
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid','8'):
  if wrong==correct or (key=='PIONEER_Q38_PEER_PHASE_ROOT' and wrong=='0'):continue
  cases.append(('build_qwen38_dispatch_image.sh',{**required,key:wrong}))
for script in ('build_qwen38_banks_driver.sh','build_qwen38_banks_client.sh','build_qwen38_persistent_rows_image.sh','build_qwen38_inference.sh'):
 for value in ('1','01','-1','invalid'):cases.append((script,{'PIONEER_Q38_PEER_PHASE_ROOT':value}))
for script in ('build_qwen38_banks_driver.sh','build_qwen38_banks_client.sh','build_qwen38_dispatch_image.sh','build_qwen38_persistent_rows_image.sh','build_qwen38_inference.sh'):
 for flag in ('PIONEER_Q38_PEER_PHASE_QUALIFICATION','PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT'):
  for value in ('1','01','-1','invalid'):cases.append((script,{flag:value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-peer-phase-root-gate-') as name:
 root=pathlib.Path(name)
 for i,(script,flags) in enumerate(cases):
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/('MUST_NOT_CREATE_'+str(i));args=[str(sdk),str(out)]
  if script=='build_qwen38_inference.sh':args=[str(sdk),str(libs),str(out)]
  r=subprocess.run(['sh',str(directory/script),*args],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} timing-return root role/format/driver/peer/client/model/Assistant refusals BEFORE SDK/libraries/output')
