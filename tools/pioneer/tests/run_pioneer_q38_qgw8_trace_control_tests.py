#!/usr/bin/env python3
"""Test fake-only trace request/copy/retention behavior; no device."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
root = here.parent
required = '''PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
_KERNEL_MODE PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY
PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_HYBRID_TRACKED_UPLOAD
PIONEER_Q38_QGW8_LAYOUT_INSPECTION PIONEER_Q38_QGW8_PROGRAMS_INSPECTION PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION
PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT'''.split()
flags = ['-D'+x+'=1' for x in required]+['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL']
flags += ['-D'+x+'=fake_'+x for x in 'api_version init_hardware init_driver uninit_driver publish_devices find_device'.split()]
compiler = os.environ.get('CXX','c++')
assert len(sys.argv) in (1,3) and (len(sys.argv)==1 or sys.argv[1]=='--sdk')
sdk = Path(sys.argv[2]) if len(sys.argv)==3 else None
assert sdk is None or sdk.is_absolute() and sdk.is_dir()
header = str(root/'PioneerQ38Qgw8TraceControlInspectionRole.h')
def check(options, success):
    result = subprocess.run([compiler, '-std=c++17', *options, '-fsyntax-only', '-x', 'c++', header], capture_output=True)
    assert (result.returncode == 0) == success, result.stderr.decode(errors='replace')[:1000]
check(flags, True)
for name in required:
    current = '-D'+name+'=1'
    if name != 'PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION':check([x for x in flags if x!=current], False)
    check([x for x in flags if x!=current]+['-D'+name+'=0'], False)
for name in ('PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION', 'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
             'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION'):
    for value in (0,1):check(flags+['-D'+name+'='+str(value)], False)
include = '#include "'+header+'"\n'
for options, suffix in ((flags, '#undef PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION\n'),
                        ([x for x in flags if x!='-DPIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION=1'],
                         '#define PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION 1\n')):
    result = subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],
                            input=include+suffix+include,text=True,capture_output=True)
    assert result.returncode != 0, 'late role change accepted'
with tempfile.TemporaryDirectory(prefix='qgw8-trace-control-',dir='/private/tmp') as name:
    tmp = Path(name); binary = tmp/'test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_qgw8_trace_control_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    execution = (here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeExecution.inc').read_text()
    signature = 'static __attribute__((noinline)) bool ReadQ38TokenNativeTraceChunk('
    assert execution.count(signature)==1
    start=execution.index(signature);end=execution.index('{',start)+1;depth=1
    while depth:
        depth+=(execution[end]=='{')-(execution[end]=='}');end+=1
    (tmp/'actual_trace_context_wrapper.inc').write_text(execution[start:end]+'\n')
    integrated=tmp/'integrated'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-I'+str(tmp),str(here/'pioneer_q38_qgw8_trace_control_context_test.cpp'),'-o',str(integrated)],check=True)
    subprocess.run([str(integrated)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    native_names='''PIONEER_Q38_QGW8_TRACE_NATIVE_131 _KERNEL_MODE PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE PIONEER_Q38_FULL_OFFLOAD_NATIVE
PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_PROGRAMS_INSPECTION PIONEER_Q38_QGW8_LAYOUT_INSPECTION
PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT TT_PIONEER_DRIVER_RELOAD'''.split()
    native=['-D'+name+'=1' for name in native_names]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL']
    native_test=tmp/'native-profile-test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*native,'-I'+str(tmp),str(here/'pioneer_q38_qgw8_trace_control_context_test.cpp'),'-o',str(native_test)],check=True)
    subprocess.run([str(native_test)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    if sdk is not None:
        cross = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
        headers = sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        obj = tmp/'control.o'
        subprocess.run([str(cross),'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                        '-Wno-unused-function','-fno-exceptions','-fno-rtti','-fstack-usage','-mcmodel=medany','-fpic',
                        *flags,'-I'+str(tmp),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                        '-c',str(here/'pioneer_q38_qgw8_trace_control_context_test.cpp'),'-o',str(obj)],check=True)
        rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        control=[row for row in rows if 'Q38Qgw8TraceControlInspection(' in row[0]]
        assert control and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in control), control
        print('PASS fake trace control RISC-V object and bounded individual control frame; NO combined-stack or device proof')
        native_obj=tmp/'native-control.o'
        subprocess.run([str(cross),'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                        '-Wno-unused-function','-fno-exceptions','-fno-rtti','-fstack-usage','-mcmodel=medany','-fpic',
                        *native,'-I'+str(tmp),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                        '-c',str(here/'pioneer_q38_qgw8_trace_control_context_test.cpp'),'-o',str(native_obj)],check=True)
        native_rows=[line.split('\t') for line in native_obj.with_suffix('.su').read_text().splitlines()]
        native_control=[row for row in native_rows if 'Q38Qgw8TraceControlInspection(' in row[0]]
        assert native_control and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in native_control), native_control
        print('PASS tag131 control RISC-V object using mock endpoints; NO native driver, combined-stack or device proof')
print('PASS fake control role isolation; actual control called with synthetic endpoints only')
