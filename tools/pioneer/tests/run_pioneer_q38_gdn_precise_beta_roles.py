#!/usr/bin/env python3
"""Syntax-only input-math role boundaries. No SDK, model or device access."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
compiler=os.environ.get('CXX','c++')
source=here/'pioneer_q38_layer29_precise_beta_bridge.cpp'
def compile_source(path,flags=()):
    return subprocess.run([compiler,'-std=c++17','-fsyntax-only','-Wall','-Wextra','-Werror',*flags,str(path)],capture_output=True,text=True)
assert compile_source(source).returncode==0
refused=0
for name in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_DRIVER_BUILD_TAG',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_SILU_PRECISION_IMAGE'):
    for value in ('0','1'):
        result=compile_source(source,['-D'+name+'='+value])
        assert result.returncode!=0 and ('Precise beta is isolated' in result.stderr or 'Isolated sigmoid candidate' in result.stderr),name
        refused+=1
with tempfile.TemporaryDirectory(prefix='pioneer-q38-beta-role-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp)
    role=here.parent/'PioneerQ38GdnPreciseBetaRole.h'
    required=('PIONEER_Q38_GDN_PRECISE_BETA_IMAGE','PIONEER_Q38_SIGMOID_PRECISION_IMAGE',
              'PIONEER_Q38_GDN_PREPARE_IMAGE','PIONEER_Q38_GDN_INPUT_IMAGE','PIONEER_Q38_GDN_INPUT_COMBINED_STAGE')
    for name in required:
        for value in (None,'0'):
            text=''.join('#define '+key+' '+('0' if key==name else '1')+'\n' for key in required if key!=name or value is not None)
            unit=root/'bad.cpp';unit.write_text(text+'#include "'+str(role)+'"\n')
            result=compile_source(unit);assert result.returncode!=0 and 'Exact separate combined-input precise beta' in result.stderr
            refused+=1
    unit=root/'late.cpp'
    unit.write_text('#define PIONEER_Q38_GDN_PREPARE_IMAGE 1\n#include "'+str(here.parent/'PioneerQ38GdnPrepare.h')+'"\n'+
                    ''.join('#define '+key+' 1\n' for key in required if key!='PIONEER_Q38_GDN_PREPARE_IMAGE')+
                    '#include "'+str(here.parent/'PioneerQ38GdnPrepare.h')+'"\n')
    result=compile_source(unit);assert result.returncode!=0 and 'already-bound legacy preparation' in result.stderr;refused+=1
print('PASS precise-beta role/1 exact positive/'+str(refused)+' defined-zero,foreign,missing and late-binding refusals;SYNTAX_ONLY_NOT_EMITTED_NATIVE_PHYSICAL_WORK_OR_TPS')
