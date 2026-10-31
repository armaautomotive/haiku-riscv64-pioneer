#!/usr/bin/env python3
"""New USER-only clock arithmetic and actual header role gates;NO device access."""
import os
import pathlib
import re
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38GenerationHostTiming.h'
gate=header.read_text().split('#ifndef PIONEER_Q38_GENERATION_HOST_TIMING_H')[0]
base={'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT':'1','PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING':'1'}
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',gate))-base.keys()
assert len(forbidden)==12
positive=negative=0
def role(values,expected,tail=''):
    global positive,negative
    source=''.join(f'#define {name} {value}\n' for name,value in values.items())+f'#include "{header}"\n'+tail
    r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',
                      '-fsyntax-only','-x','c++','-'],input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (r.returncode==0)==expected,(values,r.stderr)
    if expected:positive+=1
    else:
        assert 'error:' in r.stderr and ('Exact separate tag75' in r.stderr or 'Generation host clock DATA' in r.stderr)
        negative+=1
role(base,True)
for name in base:
    for value in (None,'0','2'):
        values=base.copy()
        if value is None:values.pop(name)
        else:values[name]=value
        role(values,False)
    role(base,False,f'#undef {name}\n#define {name} 0\n#include "{header}"\n')
for name in sorted(forbidden):
    for value in ('0','1'):role({**base,name:value},False)
    role(base,False,f'#define {name} 0\n#include "{header}"\n')
print(f'PASS tag75 clock roles/{positive} positive/{negative} missing+0+2+late+oldUSER+native+CPU+Work+TEST refusals;syntax only')
with tempfile.TemporaryDirectory(prefix='pioneer-generation-timing-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g0','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_generation_host_timing_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
