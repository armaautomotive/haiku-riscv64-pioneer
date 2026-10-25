#!/usr/bin/env python3
"""Actual DEBUG68 client/source separation;preprocessing ONLY,NO card."""
import os
import pathlib
import re
import subprocess

here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38ResidentOutputValidationSession.h'
text=header.read_text()
required={'PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT':'1','PIONEER_Q38_CPU_VALIDATION':'1'}
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',text))-set(required)
assert len(forbidden)==22
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
checks=0
include='#include "'+str(header)+'"\n'
def check(source,flags,okay):
    global checks
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-DB_DEVICE_OP_CODES_END=1000',
                           *['-D'+k+'='+v for k,v in flags.items()],'-'],input=source,text=True,
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean,timeout=30)
    assert (result.returncode==0)==okay,(flags,result.stderr)
    checks+=1
check(include,required,True)
check(include+include,required,True)
for key in required:
    missing=dict(required);del missing[key];check(include,missing,False)
    check(include+'#undef '+key+'\n'+include,required,False)
    for value in ('','0','2','-1'):
        check(include,{**required,key:value},False)
        check(include+'#undef '+key+'\n#define '+key+' '+value+'\n'+include,required,False)
for key in sorted(forbidden):
    for value in ('0','1'):
        check(include,{**required,key:value},False)
        check(include+'#define '+key+' '+value+'\n'+include,required,False)
print(f'PASS ACTUAL DEBUG68 client gates{checks}/exact CPU+client roles/native+image+old client+Work+kernel+fast-math defined0 refusal/late reinclusion;NO device execution')
