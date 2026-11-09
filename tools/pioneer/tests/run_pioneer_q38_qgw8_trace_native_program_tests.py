#!/usr/bin/env python3
"""Build-only trace program identity and immutable code checks; no driver."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) in (5,7) and (len(sys.argv)==5 or sys.argv[5]=='--sdk') and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline, trace_a, trace_b, worker=map(Path,sys.argv[1:5])
sdk=Path(sys.argv[6]) if len(sys.argv)==7 else None
assert sdk is None or sdk.is_absolute() and sdk.is_dir()
assert all(p.is_absolute() and p.is_dir() for p in (baseline,trace_a,trace_b,worker))
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
generator=root/'generate_qwen38_hybrid_programs.py'
required='''PIONEER_Q38_QGW8_TRACE_NATIVE_131 _KERNEL_MODE PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE PIONEER_Q38_FULL_OFFLOAD_NATIVE
PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_PROGRAMS_INSPECTION PIONEER_Q38_QGW8_LAYOUT_INSPECTION
PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT TT_PIONEER_DRIVER_RELOAD'''.split()
flags=['-D'+x+'=1' for x in required]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL']
role=root/'PioneerQ38Qgw8TraceNativeProgramRole.h'
def compile_role(opts,expected):
    result=subprocess.run([compiler,'-std=c++17',*opts,'-fsyntax-only','-x','c++',str(role)],capture_output=True)
    assert (result.returncode==0)==expected,result.stderr.decode(errors='replace')[:1000]
compile_role(flags,True)
for name in required[1:]:
    current='-D'+name+'=1';compile_role([x for x in flags if x!=current],False)
    compile_role([x for x in flags if x!=current]+['-D'+name+'=0'],False)
for name in '''PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128
PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION PIONEER_Q38_BF16_FULL_NATIVE
PIONEER_Q38_HYBRID_FIXED_126 PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'''.split():
    for value in (0,1):compile_role(flags+['-D'+name+'='+str(value)],False)
compile_role([x for x in flags if not x.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010129ULL'],False)
include='#include "'+str(role)+'"\n'
for options,suffix in ((flags,'#undef PIONEER_Q38_QGW8_TRACE_NATIVE_131\n'),
                       ([x for x in flags if x!='-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1'],
                        '#define PIONEER_Q38_QGW8_TRACE_NATIVE_131 1\n')):
    result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],
                          input=include+suffix+include,text=True,capture_output=True)
    assert result.returncode,'late native program selection accepted'
# Until the lifecycle and real control path exist, the existing reload
# protocol must explicitly refuse this identity even with all role flags.
reload=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
reloaded=subprocess.run([compiler,'-std=c++17',*flags,'-E','-x','c++',str(reload)],capture_output=True,text=True)
assert reloaded.returncode and 'QGW8 native candidate requires its exact tag128 reload identity' in reloaded.stderr
with tempfile.TemporaryDirectory(prefix='qgw8-trace-native-program-',dir='/private/tmp') as name:
    tmp=Path(name); outputs=[]
    for i,image in enumerate((trace_a,trace_b)):
        out=tmp/('native'+str(i)+'.h')
        p=subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-native',str(image),str(worker),str(out)],capture_output=True,text=True)
        assert p.returncode==0,p.stderr;outputs.append(out.read_bytes())
    assert outputs[0]==outputs[1]
    inspection=tmp/'inspect.h'
    subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-inspection',str(trace_a),str(worker),str(inspection)],check=True,capture_output=True)
    marker=b'#ifndef PIONEER_Q38_HYBRID_PROGRAMS_H'
    assert outputs[0].split(marker,1)[1]==inspection.read_bytes().split(marker,1)[1], 'emitted trace program bytes differ'
    denied=tmp/'wrong.h'
    p=subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-native',str(baseline),str(worker),str(denied)],capture_output=True)
    assert p.returncode and not denied.exists()
    flags+=['-D'+x+'=1' for x in ('PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
                                  'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC',
                                  'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION')]
    source=tmp/'native0.h'
    result=subprocess.run([compiler,'-std=c++17',*flags,'-I'+str(root),'-fsyntax-only','-x','c++',str(source)],capture_output=True)
    assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
    selected=root/'PioneerQ38TokenSelectedPrograms.h'
    def select(header,options):
        return subprocess.run([compiler,'-std=c++17',*options,'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"',
                               '-I'+str(root),'-fsyntax-only','-x','c++',str(selected)],capture_output=True)
    selection=select(source,flags)
    assert selection.returncode==0,selection.stderr.decode(errors='replace')[:1000]
    plan=root/'PioneerQ38Qgw8TraceRuntimePlan.h'
    plan_result=subprocess.run([compiler,'-std=c++17',*flags,'-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
                                '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"','-I'+str(root),'-fsyntax-only','-x','c++',str(plan)],capture_output=True)
    assert plan_result.returncode==0,plan_result.stderr.decode(errors='replace')[:2000]
    old=tmp/'old.h'
    subprocess.run([sys.executable,str(generator),'--qgw8-native-candidate',str(baseline),str(worker),str(old)],check=True,capture_output=True)
    assert select(old,flags).returncode, 'tag131 accepted the old QTP8 program'
    oldflags=[x for x in flags if x not in ('-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL')]
    oldflags+=['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010129ULL','-DPIONEER_Q38_QGW8_HIDDEN_NATIVE_129=1']
    assert select(source,oldflags).returncode, 'tag129 accepted QTPA trace program'
    if sdk is not None:
        cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
        headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),
                               '-idirafter',str(headers/'posix'),'-fsyntax-only','-x','c++',str(source)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"',
                               '-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                               '-fsyntax-only','-x','c++',str(selected)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
                               '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"','-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),
                               '-idirafter',str(headers/'posix'),'-fsyntax-only','-x','c++',str(plan)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:2000]
        print('PASS tag131 trace program RISC-V syntax-only; NO driver object or device')
    assert len(re.findall(rb'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',outputs[0]))==15
print('PASS distinct tag131 build-only QTPA identity, 15 pinned sections, reproducibility and role refusals; NO DRIVER OR DEVICE')
