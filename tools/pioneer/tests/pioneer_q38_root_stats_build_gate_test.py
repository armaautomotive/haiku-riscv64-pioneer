#!/usr/bin/env python3
"""Returned-counter profile is ONLY an isolated original-TC projection diagnostic."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_inference.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_ROOT_STATS_BENCH':'1','PIONEER_Q38_EXECUTION_MODE':'work',
 'PIONEER_Q38_DISPATCH_INFERENCE':'1','PIONEER_Q38_VECTOR_INFERENCE':'1',
 'PIONEER_Q38_SPAN_PRELOAD':'1','PIONEER_Q38_SPAN_FAST':'1',
 'PIONEER_Q38_CPU_POOL_PROBE':'0','PIONEER_Q38_ASSISTANT_WORKER':'0',
 'PIONEER_Q38_NATIVE_Q8_LAYOUT':'0','PIONEER_Q38_FFN_REPEAT_BENCH':'1','PIONEER_Q38_Q8_REPEAT_BENCH':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','01','-1','invalid'):
  if wrong!=correct and not(key=='PIONEER_Q38_ROOT_STATS_BENCH' and wrong=='0'):
   cases.append({**required,key:wrong})
for wrong in ('cpu-validated','checked'):
 cases.append({**required,'PIONEER_Q38_EXECUTION_MODE':wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-root-stats-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,libs,output=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(script),str(sdk),str(libs),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(flags,r.returncode,r.stderr)
  assert not sdk.exists() and not libs.exists() and not output.exists() and str(sdk) not in r.stderr
print(f'PASS {len(cases)} root-stats diagnostic-only Work/original-TC/graph/no-Assistant/no-CPU roles refuse BEFORE SDK/libraries/output')
