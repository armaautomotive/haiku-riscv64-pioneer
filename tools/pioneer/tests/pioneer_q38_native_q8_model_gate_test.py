#!/usr/bin/env python3
"""Public THREE32 SM7 qualifier is not a model/Assistant production selector."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_inference.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_NATIVE_Q8_LAYOUT':'1','PIONEER_Q38_NATIVE_Q8_MODEL_QUALIFICATION':'1',
 'PIONEER_Q38_NATIVE_Q8_GRAPH_QUALIFICATION':'0','PIONEER_Q38_Q8_REPEAT_BENCH':'0',
 'PIONEER_Q38_FFN_REPEAT_BENCH':'0','PIONEER_Q38_EXECUTION_MODE':'work',
 'PIONEER_Q38_DISPATCH_INFERENCE':'1','PIONEER_Q38_VECTOR_INFERENCE':'1',
 'PIONEER_Q38_SPAN_PRELOAD':'1','PIONEER_Q38_SPAN_FAST':'1','PIONEER_Q38_ORDERED_EXTENT_FAST':'1',
 'PIONEER_Q38_CPU_POOL_PROBE':'0','PIONEER_Q38_ASSISTANT_WORKER':'0',
 'PIONEER_Q38_PREDECODED_Q4_STICKY':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','invalid','-1'):
  if wrong!=correct:cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-native-q8-model-gate-') as name:
 root=pathlib.Path(name);sdk=root/'MUST_NOT_OPEN_SDK';libs=root/'MUST_NOT_READ_LIBS'
 for i,flags in enumerate(cases):
  out=root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(script),str(sdk),str(libs),str(out)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2 and not out.exists() and str(sdk) not in r.stderr and str(libs) not in r.stderr,(flags,r.stderr)
 r=subprocess.run(['sh',str(script),str(sdk),str(libs),str(root/'VALID_NO_BUILD')],env={**base,**required},capture_output=True,text=True)
 assert r.returncode!=2 and str(sdk) in r.stderr,r.stderr
print(f'PASS {len(cases)} public-model SM7 qualifier role/ABI/graph/CPU/Assistant isolation refusals BEFORE SDK/libs/output;valid selector reaches SDK;NO native access')
