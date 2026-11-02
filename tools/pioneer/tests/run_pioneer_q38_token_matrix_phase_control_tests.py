#!/usr/bin/env python3
"""Actual matrix packet/adapter/metadata methods;explicitly MOCK OS/native authority."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
def extract(source,signature):
    assert source.count(signature)==1
    first=source.index(signature);brace=source.index('{',first);depth,end=1,brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[first:end]
owner=(kernel/'PioneerQ38TokenNativeOwner.h').read_text()
state=(kernel/'PioneerQ38TokenNativeState.h').read_text()
adapter=extract((kernel/'PioneerQ38TokenMatrixPhaseControl.inc').read_text(),
                '[[maybe_unused]] static __attribute__((noinline)) status_t\nQ38TokenMatrixPhaseControl(')
sources={
    'actual_matrix_result.inc':extract(owner,'struct PioneerQ38TokenMatrixPhaseResult {')+';',
    'actual_matrix_nonce_owner.inc':extract(owner,'uint32_t MatrixPhaseStopNonce(unsigned worker)const{'),
    'actual_matrix_nonce_state.inc':extract(state,'uint32_t MatrixPhaseStopNonce(unsigned worker)const{'),
    'actual_matrix_control.inc':adapter,
}
with tempfile.TemporaryDirectory(prefix='pioneer-q38-matrix-control-',dir='/private/tmp') as directory:
    out=pathlib.Path(directory)
    for name,source in sources.items():(out/name).write_text(source+'\n')
    binary=out/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(out),str(here/'pioneer_q38_token_matrix_phase_control_test.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
for name,source in sources.items():print('ACTUAL_MATRIX_SOURCE '+name+' '+hashlib.sha256(source.encode()).hexdigest())
print('MOCK FD,scope,lifecycle,held read,STOP source and retention;NOT genuine native/model/physical timing or deployment qualification')
