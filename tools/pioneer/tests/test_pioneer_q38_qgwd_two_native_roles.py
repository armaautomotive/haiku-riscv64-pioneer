#!/usr/bin/env python3
"""Test native tag isolation as C++ syntax only; no driver is produced."""
import os
from pathlib import Path
import subprocess

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root=Path(__file__).resolve().parents[3]
pioneer=root/'tools/pioneer'
reload=root/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
protocol=root/'src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h'
native=('PIONEER_Q38_QGWD_NATIVE_141','_KERNEL_MODE','TT_PIONEER_DRIVER_RELOAD',
 'PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
 'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_HYBRID_TRACKED_UPLOAD','PIONEER_Q38_QGWD_PROGRAMS_INSPECTION',
 'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION','PIONEER_Q38_QGWD_CONTROL_INSPECTION',
 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-D'+x+'=1' for x in native]
tag141='-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010141ULL'
tag142='-DPIONEER_DRIVER_BUILD_TAG=0x2026111100010142ULL'
two='-DPIONEER_Q38_QGWD_TWO_NATIVE_142=1'
compiler=os.environ.get('CXX','c++')
def check(options,okay,extra=''):
 source='#define B_DEVICE_OP_CODES_END 1000\n'
 for h in (pioneer/'PioneerQ38Qgw8NativeInspectionRole.h',reload,protocol):source+='#include "'+str(h)+'"\n'
 source+=extra
 result=subprocess.run([compiler,'-std=c++17',*options,'-x','c++','-fsyntax-only','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,result.stderr[:2000]
check(flags+[tag141],True,'static_assert(PioneerQ38FullOffload::BuildTag==0x2026111000010141ULL && PioneerQ38FullOffload::MaxOutput==1,"old");\n')
check(flags+[tag142,two],True,'static_assert(PioneerQ38FullOffload::BuildTag==0x2026111100010142ULL && PioneerQ38FullOffload::MaxOutput==2,"new");\n')
check(flags+[tag142],False);check(flags+[tag141,two],False)
check([x for x in flags if x!='-DPIONEER_Q38_QGWD_NATIVE_141=1']+[tag142,two],False)
for forbidden in ('PIONEER_Q38_QGWD_TWO_CLIENT_142','PIONEER_Q38_QGWD_CLIENT_141',
                  'PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION','PIONEER_Q38_WORK_INFERENCE',
                  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
 check(flags+[tag142,two,'-D'+forbidden+'=1'],False)
check(flags+[tag142,'-DPIONEER_Q38_QGWD_TWO_NATIVE_142=0'],False)
check(flags+[tag141],False,'#define PIONEER_Q38_QGWD_TWO_NATIVE_142 1\n#include "'+str(pioneer/'PioneerQ38QgwdTwoTokenRole.h')+'"\n')
check(flags+[tag142,two],False,'#undef PIONEER_Q38_QGWD_TWO_NATIVE_142\n#include "'+str(pioneer/'PioneerQ38QgwdTwoTokenRole.h')+'"\n')
print('PASS tag141/tag142 native role, reload and protocol identity isolation; NO DRIVER OR DEVICE')
