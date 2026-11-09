#!/usr/bin/env python3
"""Verify corrected QGWC native identity and reload gates; no driver build."""
import os
from pathlib import Path
import subprocess
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
role=root/'PioneerQ38QgwcNativeRole.h'
parent=root/'PioneerQ38Qgw8NativeInspectionRole.h'
reload=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
names=('PIONEER_Q38_QGWC_NATIVE_136','_KERNEL_MODE','TT_PIONEER_DRIVER_RELOAD',
 'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
 'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
 'PIONEER_Q38_QGWC_PROGRAMS_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-D'+name+'=1' for name in names]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110900010136ULL']
def check(header,options,okay,body=None):
 source=body if body is not None else '#include "'+str(header)+'"\n'
 result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,result.stderr[:1500]
check(role,flags,True);check(reload,flags,True)
parent_flags=flags+['-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD=1','-DPIONEER_Q38_QGW8_CONTROL_INSPECTION=1',
                    '-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1']
check(parent,parent_flags,True)
check(parent,[flag for flag in parent_flags if flag!='-DPIONEER_Q38_QGWC_NATIVE_136=1'],False)
protocol=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h'
protocol_source='#define B_DEVICE_OP_CODES_END 10000\n#include "'+str(protocol)+'"\nstatic_assert(PioneerQ38HybridProtocolIdentity==136,"tag136 identity");\nstatic_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110900010136),"tag136 protocol");\n'
check(protocol,parent_flags,True,protocol_source)
protocol_test=protocol_source+'''
#include <cassert>
int main(){
 namespace F=PioneerQ38FullOffload;namespace C=PioneerQ38TokenControl;
 static_assert(F::MaxPrompt==1 && F::MaxOutput==1 && F::MaxPositions==1,"single token only");
 C::Packet p={};auto& r=p.request;r.magic=C::Magic;r.version=C::Version;r.operation=C::Start;
 r.prompts=r.limit=1;r.stopEos=1;r.eos0=PioneerQ38FullOffloadEog::Eos0;r.eos1=PioneerQ38FullOffloadEog::Eos1;p.prompt[0]=248319;
 assert(F::RequestShape(p));p.prompt[0]--;assert(!F::RequestShape(p));p.prompt[0]++;
 r.limit=2;assert(!F::RequestShape(p));r.limit=1;r.stopEos=0;assert(!F::RequestShape(p));
}
'''
with tempfile.TemporaryDirectory(prefix='qgwc-protocol-',dir='/private/tmp') as directory:
 binary=Path(directory)/'test'
 subprocess.run([compiler,'-std=c++17',*parent_flags,'-x','c++','-','-o',str(binary)],input=protocol_test,text=True,check=True)
 subprocess.run([str(binary)],check=True)
for name in names[1:]:
 reduced=[flag for flag in flags if flag!='-D'+name+'=1'];check(role,reduced,False)
 check(role,reduced+['-D'+name+'=0'],False)
for name in ('PIONEER_Q38_QGWC_PLAN_SIMULATION','PIONEER_Q38_QGWC_SRCA_ROOT_IMAGE',
 'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_QGW8_TRACE_NATIVE_131',
 'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
 check(role,flags+['-D'+name+'=1'],False)
for wrong in ('0x2026110700010128ULL','0x2026110700010134ULL','0x2026110900010135ULL'):
 check(role,flags[:-1]+['-DPIONEER_DRIVER_BUILD_TAG='+wrong],False)
 check(reload,flags[:-1]+['-DPIONEER_DRIVER_BUILD_TAG='+wrong],False)
for name in ('PIONEER_Q38_QGWC_NATIVE_136','PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128'):
 reduced=[flag for flag in flags if flag!='-D'+name+'=1'];check(reload,reduced,False)
include='#include "'+str(role)+'"\n'
check(role,flags,False,include+'#undef PIONEER_Q38_QGWC_NATIVE_136\n'+include)
include='#include "'+str(reload)+'"\n'
check(reload,flags,False,include+'#undef PIONEER_Q38_QGWC_NATIVE_136\n'+include)
print('PASS distinct tag136 role/reload guards and late or conflicting selectors; BUILD ONLY, NO device')
