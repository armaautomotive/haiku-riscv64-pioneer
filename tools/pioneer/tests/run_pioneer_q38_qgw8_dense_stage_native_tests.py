#!/usr/bin/env python3
"""Tag132 build identity and code-selection tests; no driver loading."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert len(sys.argv)==5 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,first,second,worker=map(Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (old,first,second,worker))
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
roles='''PIONEER_Q38_QGW8_TRACE_NATIVE_131 PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132 _KERNEL_MODE
PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE
PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_PROGRAMS_INSPECTION
PIONEER_Q38_QGW8_LAYOUT_INSPECTION PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT TT_PIONEER_DRIVER_RELOAD'''.split()
flags=['-D'+x+'=1' for x in roles]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010132ULL']
role=root/'PioneerQ38Qgw8TraceNativeProgramRole.h'
def compile_file(path,options,success):
    result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++',str(path)],capture_output=True,text=True)
    assert (result.returncode==0)==success,result.stderr[:1500]
compile_file(role,flags,True)
for name in roles:
    option='-D'+name+'=1'; reduced=[x for x in flags if x!=option]
    compile_file(role,reduced,False);compile_file(role,reduced+['-D'+name+'=0'],False)
for value in ('0x2026110700010131ULL','0x2026110700010128ULL'):
    compile_file(role,[x for x in flags if not x.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG='+value],False)
reload=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
def preprocess(options,success):
    result=subprocess.run([compiler,'-std=c++17',*options,'-E','-x','c++',str(reload)],capture_output=True,text=True)
    assert (result.returncode==0)==success,result.stderr[:1000]
preprocess(flags,True)
for role_name in ('PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132','PIONEER_Q38_QGW8_TRACE_NATIVE_131','TT_PIONEER_DRIVER_RELOAD'):
    preprocess([x for x in flags if x!='-D'+role_name+'=1'],False)
with tempfile.TemporaryDirectory(prefix='qgw8-dense-stage-native-',dir='/private/tmp') as directory:
    tmp=Path(directory);generator=root/'generate_qwen38_hybrid_programs.py';headers=[]
    for i,image in enumerate((first,second)):
        out=tmp/('stage'+str(i)+'.h')
        subprocess.run([sys.executable,str(generator),'--qgw8-dense-stage-native',str(image),str(worker),str(out)],check=True,capture_output=True)
        headers.append(out)
    assert headers[0].read_bytes()==headers[1].read_bytes()
    common=flags+['-D'+x+'=1' for x in ('PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
             'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC','PIONEER_Q38_QGW8_CONTROL_INSPECTION',
             'PIONEER_Q38_QGW8_SPAN_INSPECTION')]
    headerflag='-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(headers[0])+'"'
    selected=root/'PioneerQ38TokenSelectedPrograms.h'
    compile_file(selected,common+[headerflag,'-I'+str(root)],True)
    old_header=tmp/'old.h'
    subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-native',str(old),str(worker),str(old_header)],check=True,capture_output=True)
    compile_file(selected,common+['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(old_header)+'"','-I'+str(root)],False)
    old_flags=[x for x in common if x not in ('-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010132ULL')]
    old_flags+=['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL']
    compile_file(selected,old_flags+[headerflag,'-I'+str(root)],False)
    executable=tmp/'protocol'
    subprocess.run([compiler,'-std=c++17','-O1','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*common,str(here/'pioneer_q38_qgw8_trace_full_protocol_test.cpp'),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
print('PASS tag132 role, reload identity, old/new code rejection and fixed trace protocol; NO DRIVER OR DEVICE')
