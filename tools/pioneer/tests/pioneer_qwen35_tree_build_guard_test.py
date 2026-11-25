#!/usr/bin/env python3
"""A tree-settlement image is restricted to the non-loadable resident witness."""
from pathlib import Path
import subprocess
import sys
import tempfile

builder=Path(__file__).resolve().parent.parent/'build_qwen35_dataflow.py'
with tempfile.TemporaryDirectory() as temporary:
    build=Path(temporary)/'unused-build'
    cases=(
        (['--compile-tree-settlement-prototype'], 'tree settlement requires the non-loadable resident service witness'),
        (['--compile-tree-settlement-prototype','--compile-resident-service-prototype'],
         'resident service requires the non-loadable snapshot takeover witness'),
    )
    for index,(flags,message) in enumerate(cases):
        out=Path(temporary)/str(index)
        result=subprocess.run([sys.executable,'-B',str(builder),*flags,str(build),str(out)],
                              text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode!=0 and message in result.stderr and not out.exists()
print('PASS tree-settlement build requires non-loadable resident witness; OFFLINE ONLY')
