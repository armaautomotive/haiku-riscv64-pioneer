#!/usr/bin/env python3
"""BUILD role containment only,not execution or retained-closure authority."""
import os
import pathlib
import re
import subprocess
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
role=pathlib.Path(__file__).resolve().parent.parent/'PioneerQ38RetainedConvClosureRole.h'
text=role.read_text();pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
base=dict.fromkeys(set(re.findall(pattern,text))|{'PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE','__riscv'},'1')
positives=refusals=0
def check(values,okay,tail=''):
    global positives,refusals
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))+'#include "'+str(role)+'"\n'+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:positives+=1
    else:assert 'error:' in result.stderr;refusals+=1
check({},True,'static_assert(PioneerQ38RetainedConvClosureProfile==0);\n')
check(base,True,'#include "'+str(role)+'"\nstatic_assert(PioneerQ38RetainedConvClosureProfile==1);\n')
for key in sorted(base):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        if key=='PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE' and value is None:continue
        check(bad,False)
    check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n')
for key in ('_KERNEL_MODE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
            'PIONEER_Q38_LAYER_COMPONENT_DENSE','PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE',
            'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
            'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','__FAST_MATH__'):
    for value in ('0','1'):check({**base,key:value},False)
check({},False,'#define PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE 1\n#include "'+str(role)+'"\n')
check(base,False,'#undef PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE\n#include "'+str(role)+'"\n')
print(f'PASS retained-Conv BUILD role/{positives} syntax positives/{refusals} missing+zero+two+late+foreign+CPU+Work refusals;NO runtime/lifecycle/model/physical/TPS authority')
