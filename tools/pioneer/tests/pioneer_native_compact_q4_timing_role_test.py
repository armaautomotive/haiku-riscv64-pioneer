#!/usr/bin/env python3
"""Compile actual107/104 ABI,late-selection and kernel role checks only."""
import pathlib
import subprocess
import tempfile

assert __debug__
ROOT=pathlib.Path(__file__).resolve().parents[3]
protocol=ROOT/'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h'
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-q4-timing-role-'))
base=f'#define B_DEVICE_OP_CODES_END 10000\n#include "{protocol}"\n'
def compile_case(name,body,flags=(),okay=False):
    path=folder/(name+'.cpp');path.write_text(body)
    result=subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-fsyntax-only',str(path)],capture_output=True,text=True)
    assert (result.returncode==0)==okay,(name,result.stderr)
    if not okay:assert 'timed compact Q4 diagnostic107 role required' in result.stderr or 'diagnostic107 kernel identity required' in result.stderr or 'late compact Q4 timing' in result.stderr
timed=['-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1']
kernel=['-D_KERNEL_MODE','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010107ULL']
compile_case('old',base+'static_assert(sizeof(PioneerNativeCompactQ4StreamRequest)==392);',okay=True)
compile_case('timed',base+'static_assert(sizeof(PioneerNativeCompactQ4StreamRequest)==456);',timed,True)
compile_case('kernel',base,timed+kernel,True)
refused=0
for macro in ('PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE','PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE','PIONEER_Q38_WORK_INFERENCE',
    'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','__FAST_MATH__'):
    for value in (0,1):compile_case(macro+str(value),base,timed+['-D'+macro+'='+str(value)]);refused+=1
for value in (0,2):compile_case('value'+str(value),base,['-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING='+str(value)]);refused+=1
for index in range(len(kernel)):
    compile_case('missing'+str(index),base,timed+kernel[:index]+kernel[index+1:]) if index else compile_case('kernel-only',base,timed+['-D_KERNEL_MODE'])
    refused+=1
compile_case('late-add',base+'#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1\n'+base);refused+=1
compile_case('late-remove',base+'#undef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING\n'+base,timed);refused+=1
compile_case('wrong-tag',base,timed+kernel[:-1]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010104ULL']);refused+=1
print(f'PASS_Q4_TIMING_ROLE_ABI positives3 refusals{refused} fixed392vs456 lateChangesRefused1;NO_DEVICE_OR_HARDWARE_AUTHORITY')
