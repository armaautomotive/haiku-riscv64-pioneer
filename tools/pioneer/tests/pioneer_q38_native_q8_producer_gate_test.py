#!/usr/bin/env python3
"""No full-model/Assistant SM7 producer before graph/native qualification."""
import os,pathlib,subprocess,tempfile
script=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_inference.sh'
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cases=[]
for value in ('1','01','invalid','-1'):
 for mode in ('work','checked','cpu-validated'):
  cases.append({'PIONEER_Q38_NATIVE_Q8_LAYOUT':value,'PIONEER_Q38_EXECUTION_MODE':mode,
   'PIONEER_Q38_SPAN_PRELOAD':'1','PIONEER_Q38_SPAN_FAST':'1','PIONEER_Q38_VECTOR_INFERENCE':'1'})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-native-q8-producer-gate-') as name:
 root=pathlib.Path(name)
 for i,flags in enumerate(cases):
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_READ_LIBS',root/('MUST_NOT_CREATE_'+str(i))
  result=subprocess.run(['sh',str(script),str(sdk),str(libs),str(out)],env={**base,**flags},capture_output=True,text=True)
  assert result.returncode==2,(flags,result.returncode,result.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists() and str(sdk) not in result.stderr and str(libs) not in result.stderr
print(f'PASS {len(cases)} full-model/Assistant/validation SM7 producer refusals BEFORE SDK/libs/output;NO native or source selection')
