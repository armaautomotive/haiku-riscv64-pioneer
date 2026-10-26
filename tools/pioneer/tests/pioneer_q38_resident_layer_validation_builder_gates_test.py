#!/usr/bin/env python3
"""Ambient new diagnostic roles must never select/default to a Work build."""
import os
import pathlib
import subprocess
import sys
import tempfile

here=pathlib.Path(__file__).resolve().parent.parent
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image',
         'ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image',
         'resident_layer_validation_client','resident_layer_validation_native')
count=0
with tempfile.TemporaryDirectory(prefix='pioneer-layer-debug69-builder-gates-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary)
    for flag in ('PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT'):
        for value in ('0','1','2','-1','','false','0\n1'):
            for name in scripts:
                native=name=='resident_layer_validation_native'
                script=here/('build_qwen38_'+name+('.py' if native else '.sh'))
                args=[str(directory/'NO_SDK'),*([str(directory/'NO_LIBRARY')] if name=='inference' else []),str(directory/'NO_OUTPUT')]
                result=subprocess.run([sys.executable if native else 'sh',str(script),*args],
                                      capture_output=True,text=True,env={**clean,flag:value},timeout=5)
                assert result.returncode==2 and not result.stdout and not result.stderr \
                    and not (directory/'NO_OUTPUT').exists(),(name,flag,value,result.returncode,result.stderr)
                count+=1
print(f'PASS DEBUG69 ambient builder gates{count}/12native-client-legacy entries/both roles including defined0/empty/malformed refused BEFORE SDK/output access;NO Work fallback or device access')
