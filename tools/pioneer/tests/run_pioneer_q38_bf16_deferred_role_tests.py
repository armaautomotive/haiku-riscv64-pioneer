#!/usr/bin/env python3
"""Real RV32 compiler,BUILD-only opt-in separation. No image execution."""
import os,pathlib,subprocess,sys
assert __debug__ and len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parents[1]
tool=pathlib.Path(sys.argv[1])/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert tool.is_absolute() and tool.is_file()
flags={name:1 for name in ('PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE',
 'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION',
 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS',
 'PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT','PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION',
 'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL','PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS',
 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN','PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL',
 'PIONEER_Q38_BF16_PROFILE_IMAGE','PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE')}
good=bad=0
def check(values,okay,after=''):
 global good,bad
 include='#include "'+str(here/'PioneerQ38Bf16DeferredAuditRole.h')+'"\n'
 source=''.join('#define '+name+' '+str(value)+'\n' for name,value in values.items())+include+after+include
 result=subprocess.run([str(tool),'-std=c++17','-march=rv32imf','-mabi=ilp32','-ffreestanding',
  '-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,(values,after,result.stderr)
 assert not result.stdout
 if okay:assert not result.stderr;good+=1
 else:assert 'error:' in result.stderr;bad+=1
check(flags,True)
for name in flags:
 check({k:v for k,v in flags.items() if k!=name},False)
 for value in (0,2):check({**flags,name:value},False)
 check(flags,False,'#undef '+name+'\n#define '+name+' 0\n')
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','__FAST_MATH__',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
 'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
 'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE'):
 for value in (0,1):check({**flags,name:value},False)
print(f'PASS deferred prepare BUILD role positive{good}/refusals{bad}/missing-zero-wrong-late-native-worker-CPU-Work rejected;NOT actual runtime/ownership/hardware/TPS authority')
