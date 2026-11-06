#!/usr/bin/env python3
"""Compile BF16 timing BUILD roles only; no native permission or hardware."""
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv)==2
here=Path(__file__).resolve().parents[1]
tool=Path(sys.argv[1])/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert tool.is_absolute() and tool.is_file()
good=bad=0
def check(flags,okay,after='',header='PioneerQ38Bf16ProfileBuildRole.h'):
 global good,bad
 include='#include "'+str(here/header)+'"\n'
 text=''.join('#define '+name+' '+str(value)+'\n' for name,value in flags.items())+include+after+include
 result=subprocess.run([str(tool),'-std=c++17','-march=rv32imf','-mabi=ilp32','-ffreestanding',
  '-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],input=text,text=True,
  stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert (result.returncode==0)==okay,(flags,after,header,result.stderr)
 assert not result.stdout
 if okay:assert not result.stderr;good+=1
 else:assert 'error:' in result.stderr;bad+=1
common={'PIONEER_Q38_BF16_PROFILE_IMAGE':1,'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU':1,
 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION':1}
parents=[{'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE':1,'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY':1,
 'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK':1,'PIONEER_Q38_TOKEN_FRONTIER_IMAGE':1,
 'PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE':1},
 {'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE':1,'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS':1}]
for parent in parents:
 flags={**common,**parent}
 check(flags,True)
 check(flags,True,header='PioneerQ38Bf16ProfileScope.h')
 for name in flags:
  check({k:v for k,v in flags.items() if k!=name},False)
  for value in (0,2):check({**flags,name:value},False)
  check(flags,False,'#undef '+name+'\n#define '+name+' 0\n')
 for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','__FAST_MATH__',
  'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
  'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
  'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
  'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE','PIONEER_Q38_JOB_BOOT_FOLD_IMAGE','PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE'):
  for value in (0,1):check({**flags,name:value},False)
 for name in parents[1 if 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE' in parent else 0]:
  if name not in flags:check({**flags,name:0},False)
for header in ('PioneerQ38TokenFrontierImageRole.h','PioneerQ38TokenPhaseProfileImageRole.h'):
 flags={**common,**parents[0]};check(flags,True,header=header)
 selector='PIONEER_Q38_TOKEN_FRONTIER_IMAGE' if 'Frontier' in header else 'PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE'
 check({**flags,selector:0},False,header=header)
print(f'PASS BF16 timing BUILD roles:positive{good}/refusals{bad};exact ROOT/component parents;no native,CPU or Work authority')
