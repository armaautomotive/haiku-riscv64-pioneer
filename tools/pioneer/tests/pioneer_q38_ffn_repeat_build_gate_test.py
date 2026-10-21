#!/usr/bin/env python3
"""FFN repeat diagnostic must never become an Assistant/CPU/fallback mode."""
import os
import pathlib
import subprocess
import tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_inference.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required={'PIONEER_Q38_FFN_REPEAT_BENCH':'1','PIONEER_Q38_EXECUTION_MODE':'work',
 'PIONEER_Q38_VECTOR_INFERENCE':'1','PIONEER_Q38_DISPATCH_INFERENCE':'1',
 'PIONEER_Q38_SPAN_PRELOAD':'1','PIONEER_Q38_SPAN_FAST':'1',
 'PIONEER_Q38_CPU_POOL_PROBE':'0','PIONEER_Q38_ASSISTANT_WORKER':'0'}
cases=[]
for key,correct in required.items():
 for wrong in ('0','1','invalid'):
  if wrong!=correct and not(key=='PIONEER_Q38_FFN_REPEAT_BENCH' and wrong=='0'):
   cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-ffn32-build-gate-') as name:
 root=pathlib.Path(name);sdk=root/'empty-sdk';libs=root/'empty-libraries';sdk.mkdir();libs.mkdir()
 for i,flags in enumerate(cases):
  output=root/('MUST_NOT_CREATE_'+str(i))
  r=subprocess.run(['sh',str(script),str(sdk),str(libs),str(output)],env={**base,**flags},capture_output=True,text=True)
  assert r.returncode==2,(flags,r.returncode,r.stderr)
  assert not output.exists() and 'shasum:' not in r.stderr and 'cross-tools' not in r.stderr
print(f'PASS {len(cases)} FFN32 diagnostic-only Work/span/no-Assistant/no-CPU-pool/invalid-flag refusals BEFORE library/SDK content access')
