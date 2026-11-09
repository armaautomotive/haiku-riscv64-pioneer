#!/usr/bin/env python3
"""Exercise actual private trace-read methods on MOCK lifecycle/NoC only."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if sys.argv[1:] or not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('no arguments or role environment overrides permitted')
here=Path(__file__).resolve().parent;root=here.parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
compiler=os.environ.get('CXX','c++')
required=('PIONEER_Q38_QGW8_TRACE_READ_INSPECTION','_KERNEL_MODE','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
          'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
          'PIONEER_Q38_HYBRID_TRACKED_UPLOAD','PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
          'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
          'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-D'+name+'=1' for name in required]+['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL']
flags+=['-D'+name+'=fake_'+name for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')]
include='#include "'+str(root/'PioneerQ38Qgw8TraceReadInspectionRole.h')+'"\n'
def check(options,code,good):
    p=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=code,text=True,capture_output=True)
    if (p.returncode==0)!=good:raise RuntimeError(repr(options)+' '+p.stderr[:1000])
check(flags,include,True)
for flag in flags:
    check([f for f in flags if f!=flag],include,flag=='-DPIONEER_Q38_QGW8_TRACE_READ_INSPECTION=1')
for required_flag in required:
    altered=[f for f in flags if f!='-D'+required_flag+'=1']
    check(altered+['-D'+required_flag+'=0'],include,False)
for forbidden in ('PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
                  'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
                  'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE',
                  'PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION','PIONEER_Q38_HYBRID_FULL_NATIVE',
                  'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                  'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    for value in (0,1):check(flags+['-D'+forbidden+'='+str(value)],include,False)
check(flags,include+'#undef PIONEER_Q38_QGW8_TRACE_READ_INSPECTION\n'+include,False)
check([f for f in flags if f!='-DPIONEER_Q38_QGW8_TRACE_READ_INSPECTION=1'],
      include+'#define PIONEER_Q38_QGW8_TRACE_READ_INSPECTION 1\n'+include,False)
owner=(kernel/'PioneerQ38TokenNativeOwner.h').read_text()
hidden=(kernel/'PioneerQ38TokenHiddenRead.inc').read_text()
trace=(kernel/'PioneerQ38Qgw8TraceReadInspection.inc').read_text()
def extract(source,signature):
    assert source.count(signature)==1,signature
    begin=source.index(signature);brace=source.index('{',begin);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[begin:end]
signatures=((owner,'template<class IO> bool BootTime('),(owner,'template<class IO,class Result> bool HoldGuard('),
            (owner,'template<class IO> bool ReadIdle('),(owner,'template<class IO> bool ReadFrontier('),
            (hidden,'bool HiddenReadOutput('),(hidden,'template<class IO> bool ReadHiddenPacket('),
            (hidden,'template<class IO> __attribute__((noinline)) bool ReadHiddenChunk('),
            (trace,'bool TraceReadOutput('),(trace,'template<class IO> bool ReadTracePacket('),
            (trace,'template<class IO> __attribute__((noinline)) bool ReadTraceChunk('))
methods=[extract(source,signature) for source,signature in signatures]
records='\n'.join(extract(owner,'struct '+name+' {')+';' for name in ('PioneerQ38TokenReadResult','PioneerQ38TokenHiddenReadResult'))
sources=(kernel/'PioneerQ38TokenNativeOwner.h',kernel/'PioneerQ38TokenHiddenRead.inc',kernel/'PioneerQ38Qgw8TraceReadInspection.inc',
         root/'PioneerQ38Qgw8LayerTraceGeometry.h',root/'PioneerQ38Qgw8LayerTraceAssembler.h',
         root/'PioneerQ38Qgw8LayerTracePacket.h',root/'PioneerQ38Qgw8LayerTraceHandoff.h',
         here/'pioneer_q38_hidden_read_method_test.cpp',here/'pioneer_q38_qgw8_trace_read_method_test.cpp')
pins={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
with tempfile.TemporaryDirectory(prefix='qgw8-trace-read-',dir='/private/tmp') as name:
    tmp=Path(name);(tmp/'actual_hidden_records.inc').write_text(records+'\n')
    (tmp/'actual_hidden_methods.inc').write_text('unsigned traceReadCursor=0;\n'+'\n'.join(methods)+'\n')
    binary=tmp/'test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(tmp),str(here/'pioneer_q38_qgw8_trace_read_method_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'quarantine_size_mb=32'})
for path,digest in pins.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
print('PASS extracted actual methods, MOCK NoC/lifecycle only; NO native/device/speed authority')
