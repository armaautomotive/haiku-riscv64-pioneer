#!/usr/bin/env python3
"""Distinct BUILD/DATA containment and malformed producer early refusals."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'pioneer_q38_q4_constant_issue_build_gate_test.py')],check=True)
role=here.parent/'PioneerQ38TokenMatrixPhaseRole.h';data=here.parent/'PioneerQ38TokenMatrixPhase.h'
image='PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE';test='PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST'
base={k:'1' for k in (image,'PIONEER_Q38_TOKEN_MATRIX_PHASE_PARENT_BINDING',
 'PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE','PIONEER_Q38_Q4_CONSTANT_ISSUE','PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING',
 'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE','PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH',
 'PIONEER_Q38_STATIC_VECTOR','PIONEER_Q38_REPLAY_OPERAND','PIONEER_Q38_NATIVE_OPERAND_STORE','PIONEER_Q38_VECTOR_INLINE')}
positive=negative=0
include='#include "'+str(role)+'"\n'
def syntax(values,okay,body=None,selection=1):
 global positive,negative
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))
 source+=body if body is not None else include+include+(f'static_assert(PioneerQ38TokenMatrixPhaseSelection=={selection});\n' if okay else '')
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
  input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,(values,body,result.stderr)
 if okay:positive+=1
 else:
  assert 'error:' in result.stderr;negative+=1
syntax({},True,selection=0);syntax(base,True);syntax({test:'1'},True,selection=2)
syntax({test:'1'},True,'#include "'+str(data)+'"\n',selection=2)
text=role.read_text()
foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',text))-set(base)-{test}
# Freestanding/MIXED are forbidden for pure DATA,not for the actual shader.
# Keep that distinction explicit instead of treating every branch's
# exclusions as a global blacklist. Exercise the real BUILD combination.
build_foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',
 text.split('#error Matrix phase BUILD/DATA cannot alias')[0]))-set(base)-{test}
syntax({**base,'PIONEER_ATTENTION_FREESTANDING':'1','PIONEER_Q38_MIXED_CAPSULE_IMAGE':'1'},True)
syntax({**base,'PIONEER_Q38_VECTOR_INLINE':''},True)
for key in sorted(base):
 for value in (None,'0','2'):
  bad=base.copy()
  if value is None:del bad[key]
  else:bad[key]=value
  syntax(bad,False)
 syntax(base,False,include+'#undef '+key+'\n'+include)
 syntax(base,False,include+'#undef '+key+'\n#define '+key+' 0\n'+include)
for key in sorted(build_foreign|{test}):
 for value in ('0','1'):
  syntax({**base,key:value},False)
  syntax(base,False,include+'#define '+key+' '+value+'\n'+include)
for key in sorted(foreign|set(base)):
 for value in ('0','1'):
  syntax({test:'1',key:value},False)
for value in ('0','2'):syntax({test:value},False)
syntax({},False,'#include "'+str(data)+'"\n')
syntax({},False,include+'#define '+image+' 1\n'+include,selection=0)
syntax({test:'1'},False,include+'#undef '+test+'\n'+include,selection=2)
syntax({},False,include+'#define '+test+' 1\n'+include,selection=0)
print(f'PASS_MATRIX_PHASE_ROLE positives{positive} missing,zero,two,late,native,USER,old-profile,shader-mix,CPU+WorkRefusals{negative};NO execution grant',flush=True)
builder=here.parent/'build_qwen38_token_matrix_phase_worker.py'
with tempfile.TemporaryDirectory(prefix='pioneer-matrix-phase-early-',dir='/private/tmp') as tmp:
 root=pathlib.Path(tmp);sdk=root/'ABSENT_SDK';out=root/'MUST_NOT_EXIST';cases=0
 for args in ([],[str(sdk)],['--unknown',str(sdk),str(out)],
              ['--inspect-candidate','--inspect-candidate',str(sdk),str(out)],[str(sdk),'relative-output']):
  result=subprocess.run([sys.executable,str(builder),*args],capture_output=True)
  assert result.returncode and not out.exists();cases+=1
 for key in (image,test,'PIONEER_Q38_TOKEN_MATRIX_PHASE_PARENT_BINDING','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
  for value in ('','0','1','2','-1'):
   result=subprocess.run([sys.executable,str(builder),str(sdk),str(out)],capture_output=True,env={**os.environ,key:value})
   assert result.returncode and not out.exists();cases+=1
 print(f'PASS_MATRIX_PHASE_BUILDER_PRESDK_REFUSALS {cases} no SDK/output/device invocation',flush=True)
