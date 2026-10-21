#!/usr/bin/env python3
"""Host thread tuning must never mix with validation/profilers/Assistant."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_inference.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_MODEL_THREAD_SWEEP':'1','PIONEER_Q38_EXECUTION_MODE':'work',
 'PIONEER_Q38_DISPATCH_INFERENCE':'1','PIONEER_Q38_VECTOR_INFERENCE':'1',
 'PIONEER_Q38_SPAN_PRELOAD':'1','PIONEER_Q38_SPAN_FAST':'1','PIONEER_Q38_ORDERED_EXTENT_FAST':'1',
 'PIONEER_Q38_CPU_POOL_PROBE':'0','PIONEER_Q38_ASSISTANT_WORKER':'0','PIONEER_Q38_NATIVE_Q8_LAYOUT':'0',
 'PIONEER_Q38_ROOT_STATS_BENCH':'0','PIONEER_Q38_FFN_REPEAT_BENCH':'0','PIONEER_Q38_Q8_REPEAT_BENCH':'0',
 'PIONEER_Q38_BENCHMARK_HOST':'0','PIONEER_Q38_MODEL_ROOT_PROFILE':'0','PIONEER_Q38_MODEL_NODE_PROFILE':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid','cpu-validated','checked'):
  if wrong==correct or (key=='PIONEER_Q38_MODEL_THREAD_SWEEP' and wrong=='0'):continue
  cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-thread-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(script),str(sdk),str(libs),str(out)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} thread-sweep role/validation/Assistant/profile/SM7/host refusals BEFORE SDK/libraries/output')
