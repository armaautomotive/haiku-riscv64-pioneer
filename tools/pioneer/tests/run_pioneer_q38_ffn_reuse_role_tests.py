#!/usr/bin/env python3
"""Preprocessor isolation for the offline FFN input-reuse experiment."""
import ast
import os
from pathlib import Path
import subprocess

here=Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
tree=ast.parse((here/'build_pioneer_q38_recurrent_controller_component.py').read_text())
roles=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
           and any(isinstance(t,ast.Name) and t.id=='roles' for t in n.targets))
base={r:1 for r in roles}
target='PIONEER_Q38_FFN_INPUT_REUSE_INSPECTION'
source=here.parent/'blackhole_q38_bf16_projection_root.inc'
def check(definitions,okay):
    result=subprocess.run([os.environ.get('CXX','c++'),'-E','-x','c++',
                           *['-D'+k+'='+str(v) for k,v in definitions.items()],str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert (result.returncode==0)==okay,(definitions,result.stderr)
    if not okay:assert 'FFN input reuse is a separate offline recurrent-root inspection only' in result.stderr
check(base,True)
check({**base,target:1},True)
for value in (0,2):check({**base,target:value},False)
for parent in ('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWD_ROOT_IMAGE',
               'PIONEER_Q38_LAYER_COMPONENT_RECURRENT','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (None,0,2):
        bad={**base,target:1}
        if value is None:del bad[parent]
        else:bad[parent]=value
        check(bad,False)
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                  'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                  'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
    for value in (0,1):check({**base,target:1,forbidden:value},False)
print('PASS FFN reuse inspection role isolation; NO NATIVE, WORK OR DEVICE AUTHORITY')
