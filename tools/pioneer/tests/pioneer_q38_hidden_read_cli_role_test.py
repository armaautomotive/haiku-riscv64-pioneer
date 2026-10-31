#!/usr/bin/env python3
"""Held-hidden fixture/CLI isolation;no source authentication/model execution."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
header=here/'pioneer_q38_token_native_hidden_read_role.h'
runner=here/'run_pioneer_q38_token_native_initialization_tests.py'
source=header.read_text()
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',source))
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',source))-required
assert len(required)==11 and len(forbidden)==9 and not required & forbidden
base=dict.fromkeys(required,1);accepted=refused=0
cases=[(base,True),({},False)]
for name in sorted(required):
    cases.append(({k:v for k,v in base.items() if k!=name},False))
    cases += [({**base,name:value},False) for value in (0,2)]
for name in sorted(forbidden):cases += [({**base,name:value},False) for value in (0,1,2)]
for flags,okay in cases:
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-x','c++','-fsyntax-only',
                           *['-D'+k+'='+str(v) for k,v in sorted(flags.items())],'-'],
                          input='#include "'+str(header)+'"\nint main(){}\n',text=True,
                          stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay,result.stderr
    if not okay:assert 'Explicit original G3 first-layer/cancel/STOP/HOLD/Read0/full-END hidden readout TEST required' in result.stderr
    accepted+=okay;refused+=not okay
with tempfile.TemporaryDirectory(prefix='pioneer-hidden-read-gates-',dir='/private/tmp') as temporary:
    tmp=pathlib.Path(temporary);prefix=tmp/'must-not-exist.bin'
    common=['/unopened/zero','--execution-end','/unopened/model','/unopened/old-worker','/unopened/g3',str(prefix)]
    hidden='--held-hidden-readout';root='--root-frontend-snapshots';pushed='--pushed-context'
    cases=[
        (common+[hidden],'Held hidden TEST requires'),
        (common+[pushed,hidden],'Held hidden TEST requires'),
        (common+[root,hidden],'ROOT frontend TEST requires'),
        (common+[pushed,root,hidden,hidden],'duplicate held hidden TEST selector'),
        (common+[pushed,root,hidden,'--backend-finish'],'Held hidden TEST requires'),
        (common+[pushed,root,hidden,'--control-adapter'],'private control TEST requires'),
        (common+[pushed,root,hidden,'--hold-recheck-pending'],'historical pending-PC selector retired'),
        (common+[pushed,root,hidden,'--precise-context'],'QTP3 cannot mix with QTP2'),
        (common+[pushed,root,root,hidden],'duplicate ROOT frontend TEST selector'),
        (common+[pushed,root,hidden,'--unknown'],'ROOT frontend TEST requires'),
    ]
    for mode in ('--execution-end-reload','--execution-first-layer','--execution-hold-first-layer',
                 '--execution-end-full-flow','--execution-full-flow','--execution-readout-empty'):
        args=list(common);args[1]=mode;cases.append((args+[pushed,root,hidden],'ROOT frontend TEST requires'))
    before=sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*'))
    for args,expected in cases:
        result=subprocess.run([sys.executable,str(runner),*args],text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode and expected in result.stderr and not result.stdout,result.stderr
        assert not prefix.exists() and sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*'))==before
    print(f'PASS held-hidden TEST gates/{accepted} role positive/{refused} missing+zero+two+foreign role refusals/{len(cases)} early CLI refusals/no output/compiler/runtime creation;NO native/model/physical/TPS grant')
