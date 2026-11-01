#!/usr/bin/env python3
"""Syntax/binding refusals ONLY;no emitted code,owner or execution grant."""
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent.parent
role=here/'PioneerQ38FlatMatrixGrammarRole.h'
include='#include "'+str(role)+'"\n'
common=[os.environ.get('CXX','c++'),'-std=c++11','-Werror','-fsyntax-only','-x','c++','-']
base={k:'1' for k in ('PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE','PIONEER_Q38_JOB_BOOT_FOLD_IMAGE',
 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION','__riscv')}
base['__riscv_xlen']='32'
positive=negative=0
def check(values,okay,tail='',selected=1):
    global positive,negative
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+include
    source+='static_assert(PioneerQ38FlatMatrixGrammarSelection=='+str(selected)+',"expected DATA selection");\n'+tail
    result=subprocess.run(common,input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:positive+=1
    else:assert 'error:' in result.stderr;negative+=1
for selected,other in (('PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'),
                       ('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')):
    good={**base,selected:'1'};check(good,True)
    for key in good:
        for value in (None,'0','2'):
            bad=good.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            check(bad,False)
        check(good,False,'#undef '+key+'\n#define '+key+' 0\n'+include)
    check(good,False,'#undef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE\n'+include)
    for value in ('0','1'):check({**good,other:value},False)
    check({**good,'__riscv_xlen':'64'},False)
    for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
                'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS',
                'PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE',
                '__FAST_MATH__'):
        for value in ('0','1'):check({**good,key:value},False)
check({},False,'#define PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE 1\n'+include,selected=0)
print(f'PASS flat grammar/{positive} syntax positives/{negative} missing+zero+two+late+foreign+native+CPU+Work refusals;'
      'HOST faked architecture macros are syntax arguments ONLY,not emitted RV32/owner/model/device/TPS proof')
