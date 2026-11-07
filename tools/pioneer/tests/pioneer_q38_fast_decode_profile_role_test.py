#!/usr/bin/env python3
"""Actual SDK syntax refusals for the isolated fast-decode ROOT build."""
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent.parent;build=Path(sys.argv[1]);assert build.is_absolute()
header='#include "PioneerQ38FastDecodeProfileBuildRole.h"\n'
positive=refused=0
common=('PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
        'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION')
forbidden=('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','__FAST_MATH__',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
 'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE','PIONEER_Q38_BF16_PROFILE_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
 'PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE','PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE')
def name(flag):return flag[2:].split('=',1)[0]
for kind in ('token','recurrent','dense'):
 command=(build/('compile-'+kind+'-argv.txt')).read_text().splitlines()
 cc=command[0];defines=[f for f in command if f.startswith('-D')]
 assert any(name(f)=='PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE' for f in defines)
 base=[cc,'-std=c++11','-march=rv32imf','-mabi=ilp32','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-I',str(here)]
 def check(flags,okay,source=header):
  global positive,refused
  result=subprocess.run([*base,*flags,'-'],input=source,capture_output=True,text=True)
  assert (result.returncode==0)==okay,(kind,flags,result.stdout,result.stderr)
  if okay:assert not result.stderr;positive+=1
  else:refused+=1
 check(defines,True)
 for value in ('0','2'):
  check([f for f in defines if name(f)!='PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE']+
        ['-DPIONEER_Q38_FAST_DECODE_PROFILE_IMAGE='+value],False)
 required=common+(('PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE','PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE',
  'PIONEER_Q38_TOKEN_FRONTIER_IMAGE','PIONEER_Q38_TOKEN_CANCEL_BOUNDARY','PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK',
  'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE') if kind=='token' else
  ('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS'))
 for macro in required:
  assert any(name(f)==macro for f in defines)
  check([f for f in defines if name(f)!=macro],False)
 for macro in forbidden:check([*defines,'-D'+macro+'=1'],False)
 check(defines,False,header+'#undef PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE\n'+header)
 initial=[f for f in defines if name(f)!='PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE']
 check(initial,False,header+'#define PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE 1\n'+header)
 check(initial,True)
print(f'PASS_FAST_DECODE_BUILD_ROLE positive{positive} refused{refused} actualSDK_root_and_both_components_late_alias_CPU_Work_native_BF16_prepared_refused;NO_DEVICE')
