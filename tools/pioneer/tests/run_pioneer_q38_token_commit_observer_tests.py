#!/usr/bin/env python3
"""Extract actual HOST observer guards,not firmware;NO model/device execution."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
source=(here/'pioneer_q38_token_flow_test.cpp').read_text()
start=source.index(' void Store(uint32_t a,uint32_t value)override{')
at=source.index('{',start);depth=1;end=at+1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
store=source[start:end]
token=source[source.index(' void TokenPacket('):]
begin=token.index('assert(')+len('assert(');depth=1;end=begin
while depth:
    depth+=(token[end]=='(')-(token[end]==')');end+=1
publication=token[begin:end-1]
assert 'flips==position+1 && ActualPrivate(18)==position+1 && ActualPrivate(19)==((position+1)&1)' in publication
with tempfile.TemporaryDirectory(prefix='pioneer-token-observer-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp);binary=root/'test'
    (root/'actual_token_observer_store.inc').write_text(store+'\n')
    (root/'actual_token_observer_publication.inc').write_text('return '+publication+';\n')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g0','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(root),str(here/'pioneer_q38_token_commit_observer_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
