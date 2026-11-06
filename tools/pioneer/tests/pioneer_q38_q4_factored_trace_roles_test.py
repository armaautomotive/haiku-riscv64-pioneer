#!/usr/bin/env python3
"""Default-off diagnostic/source roles only;no device,network or native grant."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
assert __debug__ and len(sys.argv)==2 and not os.environ.get('PYTHONOPTIMIZE')
sdk=Path(sys.argv[1]);here=Path(__file__).resolve().parent.parent
cc=os.environ.get('CXX','c++');cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
refused=0
with tempfile.TemporaryDirectory(prefix='pioneer-trace-roles-') as temporary:
    source=Path(temporary)/'probe.cpp'
    for header in ('PioneerQ38Q4FactoredTrace.h','PioneerQ38Q4FactoredTraceReference.h','PioneerQ38Q4FactoredTraceReport.h'):
        source.write_text('#include "'+header+'"\nint main(){return 0;}\n')
        base=[cc,'-std=c++17','-I'+str(here),'-DPIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC=1','-DPIONEER_Q38_Q4_FACTORED_TRACE_REPORT=1','-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1','-fsyntax-only',str(source)]
        subprocess.run(base,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        macros=('_KERNEL_MODE','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE')
        if header!='PioneerQ38Q4FactoredTraceReport.h':macros+=('__FAST_MATH__',)
        for macro in macros:
            for value in (0,1):
                result=subprocess.run([*base,'-D'+macro+'='+str(value)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
                assert result.returncode,(header,macro,value)
                refused+=1
        for macro in ('PIONEER_Q38_WORK_INFERENCE','_KERNEL_MODE'):
            source.write_text('#include "'+header+'"\n#define '+macro+' 1\n#include "'+header+'"\n')
            result=subprocess.run(base,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            assert result.returncode,(header,'late',macro)
            refused+=1
    source.write_text('#include "PioneerQ38Q4FactoredTrace.h"\n')
    for flag in ('-UPIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC','-DPIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC=0'):
        result=subprocess.run([*base,flag],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        assert result.returncode
        refused+=1
    base=[cross,'-std=c++17','-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1','-fsyntax-only',str(here/'blackhole_q38_factored_q4_trace.cpp')]
    for flags in ([],['-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=0'],['-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=2'],['-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=1','-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1'],['-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=1','-DPIONEER_Q38_FACTORED_Q4_MATRIX_BUILD=1']):
        args=[*base,*flags]
        result=subprocess.run(args,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        assert result.returncode
        refused+=1
print('PASS_TRACE_ROLE_REFUSALS',refused,'wrongMissingLate_kernel_Work_native_CPU_fastMath_buildAliases;NO_HARDWARE')
