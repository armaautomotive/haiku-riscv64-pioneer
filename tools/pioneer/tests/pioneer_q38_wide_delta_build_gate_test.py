#!/usr/bin/env python3
"""Actual standalone-image CLI and instruction/bridge role refusals;no card IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
builder=here.parent/'build_qwen38_wide_delta_local_image.py'
image_cases=role_cases=0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-wide-delta-gates-',dir='/private/tmp') as temporary:
 root=pathlib.Path(temporary);sdk=root/'MUST_NOT_OPEN_SDK'
 def refuse_build(tail=None,roles=None):
  global image_cases
  output=root/('forbidden-'+str(image_cases))
  args=[str(sdk),str(output)] if tail is None else tail
  result=subprocess.run([sys.executable,str(builder),*args],env={**os.environ,**(roles or {})},capture_output=True,text=True)
  assert result.returncode and not output.exists() and not output.is_symlink()
  assert 'OFFLINE_BUILD_ONLY' not in result.stdout
  image_cases+=1
 for tail in ([],[str(sdk)],[str(sdk),str(root/'out'),'bad'],[str(sdk),str(root/'out'),'--inspect-candidate','extra'],['relative-sdk',str(root/'out')],[str(sdk),'relative-out']):refuse_build(tail)
 for key in ('PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
  for value in ('0','1','bad'):refuse_build(roles={key:value})
 snippet=root/'math.cpp';snippet.write_text('#include "PioneerQ38WideDeltaMath.h"\n#include "PioneerQ38WideDeltaLocalLayout.h"\n')
 def compile_case(source,defines,valid=False):
  global role_cases
  result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-I'+str(here.parent),*['-D'+key+'='+value for key,value in defines.items()],str(source)],capture_output=True,text=True)
  assert (result.returncode==0)==valid,(defines,result.stderr)
  role_cases+=1
 for role in ('PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE','PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST'):
  compile_case(snippet,{role:'1'},valid=True)
  for value in ('0','2'):compile_case(snippet,{role:value})
 compile_case(snippet,{})
 compile_case(snippet,{'PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE':'1','PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST':'1'})
 for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','__FAST_MATH__'):
  compile_case(snippet,{'PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST':'1',key:'1'})
 bridge=here/'pioneer_q38_layer29_wide_emitted_delta_bridge.cpp'
 compile_case(bridge,{'PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC':'1'},valid=True)
 compile_case(bridge,{})
 compile_case(bridge,{'PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC':'0'})
 for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST','PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC','PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC'):
  compile_case(bridge,{'PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC':'1',key:'1'})
print('PASS actual wide-delta image CLI:'+str(image_cases)+' refusals before output creation/'+str(role_cases)+' instruction+bridge syntax roles incl3 positive boundaries;NO model arithmetic/native/device/Work/TPS')
