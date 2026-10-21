#!/usr/bin/env python3
"""Exact27 direct Q4 original-layout qualification is isolated from every other role."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
driver={"PIONEER_Q38_Q4_NIBBLE_QUALIFICATION":"1","PIONEER_Q38_SENTINEL_POLL_QUALIFICATION":"1","PIONEER_Q38_WORK_SENTINEL_POLL":"1","PIONEER_Q38_DIRECT_SRCB_QUALIFICATION":"1","PIONEER_Q38_WORK_DIRECT_SRCB":"1","PIONEER_Q38_AFFINE_RAWDOT_QUALIFICATION":"1","PIONEER_Q38_WORK_AFFINE_REPLAY":"1","PIONEER_Q38_WORK_DOT_ZERO_ELISION":"1","PIONEER_Q38_BLOCK_SCALE_REUSE":"1","PIONEER_Q38_SCALE_REUSE_QUALIFICATION":"1","PIONEER_Q38_BLOCK_AFFINE_DST":"1","PIONEER_Q38_BLOCK_AFFINE_QUALIFICATION":"1","PIONEER_Q38_ROOT_CYCLE_PROFILE":"1","PIONEER_Q38_INLINE_CONFIG":"1","PIONEER_Q38_WORK_DEFER_AFFINE":"1","PIONEER_Q38_WORK_COMPACT_OUTPUT":"1","PIONEER_Q38_NATIVE_OPERAND_STORE":"1","PIONEER_Q38_WORK_ZERO_MINIMUM":"1","PIONEER_Q38_WORK_INPUT_SUM_CACHE":"1","PIONEER_Q38_DISPATCH_PROFILE":"0","PIONEER_Q38_DISPATCH_BATCHED_PIO":"0","PIONEER_Q38_COMPACT_ACTIVATIONS":"1","PIONEER_Q38_REUSE_ACTIVATIONS":"1","PIONEER_Q38_REUSE_CROSS_QUANT":"1","PIONEER_Q38_WORK_MODE":"1","PIONEER_Q38_WORK_POSTED_STAGE":"1","PIONEER_Q38_WORK_COHORT_CACHE":"1","PIONEER_Q38_Q4_TWO_PHASE":"1","PIONEER_Q38_SPAN_PRELOAD":"1","PIONEER_Q38_SPAN_FAST":"1","PIONEER_Q38_SPAN_REUSE_CODE":"1","PIONEER_Q38_INT_SFPU":"1","PIONEER_Q38_PIPELINED_MATRIX":"1","PIONEER_Q38_STATIC_VECTOR":"1","PIONEER_Q38_CACHED_AFFINE":"1","PIONEER_Q38_REPLAY_OPERAND":"1","PIONEER_Q38_WORK_SKIP_DOT_CHECK":"1","PIONEER_Q38_WORK_OUTPUT_DMA":"1","PIONEER_Q38_QUEUED_MATRIX_ENTRY":"1","PIONEER_Q38_LOCAL_DELTA_PROBE":"1","PIONEER_Q38_WORK_SPARSE_EXPORT":"1","PIONEER_Q38_NORMAL_CLOCK":"1","PIONEER_Q38_PANEL_DRIVER":"10","PIONEER_DRIVER_RELOAD":"1","PIONEER_DRIVER_BUILD_TAG":"2026102100010027","PIONEER_Q38_WORK_Q4_NIBBLE":"0","PIONEER_Q38_WORK_SIGNED_MAG2":"0","PIONEER_Q38_PEER_PHASE_QUALIFICATION":"0","PIONEER_Q38_COMMAND_REUSE_QUALIFICATION":"0","PIONEER_Q38_WORK_ROOT_COMMAND_REUSE":"0","PIONEER_Q38_TILE_PREFETCH_QUALIFICATION":"0","PIONEER_Q38_WORK_TILE_PREFETCH":"0","PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION":"0","PIONEER_Q38_WORK_RAW_UNPACK_CACHE":"0","PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION":"0","PIONEER_Q38_WORK_FRONTEND_BARRIER":"0","PIONEER_Q38_BATCH4_FETCH_QUALIFICATION":"0","PIONEER_Q38_WORK_BATCH4_FETCH":"0","PIONEER_Q38_WORK_BATCH_RAW_INPUT":"0","PIONEER_Q38_HOST_PHASE_PROFILE":"0","PIONEER_Q38_GDN_MUTABLE_STATE":"0","PIONEER_Q38_RESIDENT_GDN":"0","PIONEER_Q38_LOCAL_DELTA_GGML":"0","PIONEER_Q38_ORDERED_EXTENT_FAST":"0","PIONEER_Q38_Q4_DIRECT_QUALIFICATION":"1"}
client={"PIONEER_Q38_Q4_NIBBLE_REFERENCE_CLIENT":"1","PIONEER_Q38_SENTINEL_REFERENCE_CLIENT":"1","PIONEER_Q38_PANEL_CLIENT":"4","PIONEER_Q38_VECTOR_CLIENT":"1","PIONEER_Q38_WORK_CLIENT":"1","PIONEER_Q38_ROOT_CYCLE_CLIENT":"1","PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT":"0","PIONEER_Q38_COMMAND_REFERENCE_CLIENT":"0","PIONEER_Q38_PREFETCH_REFERENCE_CLIENT":"0","PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT":"0","PIONEER_Q38_FRONTEND_REFERENCE_CLIENT":"0","PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT":"0","PIONEER_Q38_BATCH4_REFERENCE_CLIENT":"0","PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT":"0","PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT":"0","PIONEER_Q38_GROUP32_REFERENCE_CLIENT":"0","PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT":"0","PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT":"0","PIONEER_Q38_Q4_DIRECT_REFERENCE_CLIENT":"1"}
cases=[]
for script,role in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for key,correct in role.items():
  for wrong in ('0','1','01','-1','invalid','2026102100010025','2026102100010026','2026102100010028'):
   if script=='build_qwen38_banks_client.sh' and key=='PIONEER_Q38_Q4_DIRECT_REFERENCE_CLIENT' and wrong=='0':continue
   if wrong!=correct:cases.append((script,{**role,key:wrong}))
for script in ('build_qwen38_inference.sh','build_qwen38_dispatch_image.sh','build_qwen38_persistent_rows_image.sh'):
 for flag in ('PIONEER_Q38_Q4_DIRECT_QUALIFICATION','PIONEER_Q38_Q4_DIRECT_REFERENCE_CLIENT'):
  for wrong in ('1','01','-1','invalid'):cases.append((script,{flag:wrong}))
# A foreign debug role cannot be smuggled into an otherwise complete driver.
cases.append(('build_qwen38_banks_driver.sh',{**driver,'PIONEER_Q38_Q4_NIBBLE_REFERENCE_CLIENT':'1'}))
for script,role in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for value in ('1','01','-1','invalid'):cases.append((script,{**role,'PIONEER_Q38_WORK_Q4_DIRECT_DECODE':value}))
for script,role in [('build_qwen38_banks_driver.sh',driver),('build_qwen38_banks_client.sh',client)]:
 for value in ('1','01','-1','invalid'):cases.append((script,{**role,'PIONEER_Q38_WORK_LOCAL_Q4':value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-q4-native-gates-') as name:
 root=pathlib.Path(name);empty=root/'EMPTY_PATH';empty.mkdir()
 for script,flags in cases:
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/'MUST_NOT_CREATE_OUTPUT'
  args=[str(sdk),str(out)] if script!='build_qwen38_inference.sh' else [str(sdk),str(libs),str(out)]
  r=subprocess.run(['/bin/sh',str(directory/script),*args],env={**base,**flags,'PATH':str(empty)},capture_output=True,text=True)
  assert r.returncode==2 and 'not found' not in r.stderr,(script,flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists()
print(f'PASS {len(cases)} exact27 direct Q4 driver/debug/native/model/root/format/partial/mixed refusals BEFORE ANY external command/SDK/output;reserved27 cannot label old/nibble-only firmware')
