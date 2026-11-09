#!/usr/bin/env python3
"""Test proposed dense-Q8 journal geometry; no device or native owner."""
import os
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
if len(sys.argv) not in (1,2):raise SystemExit('optional absolute pinned QGWD worker directory')
worker=Path(sys.argv[1]) if len(sys.argv)==2 else None
if worker:
    assert worker.is_absolute() and worker.is_dir()
    pins={'program.elf':'b9ab4753f2e99734cdbeb16b4fc0077688784c8b841a7e7aba128981a610b6f8',
     'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
     'globalworker.bin':'cbe9eb61d3d231c254fc0a86f60bbe8b8e89761091fbad12c779e5245c9df85c',
     'bf16producer.bin':'531453b71c59409485087339f36cfd0b48700c7edb53b2b4bcdfd3dea913acd8',
     'dense.bin':'8d0751a2ca303c122c85bf1952709ff36d8fb0549978f731391fc9ce81845206',
     'cachedfpu.bin':'b1f1de4a7265ce05f33435c7ab65b5c22b9dbd483ea42febff7361c6cdeed562',
     'ffn.bin':'94ee6dc85ef68047880bc54b1148925ffd30489648ce8c7a6f0acf67649018c7',
     'service.bin':'0456bed771cf6db9185fec31a185a787bcca0ec22c5341523779b94c8da91048',
     'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
     'text.bin':'f912ca99337a3032deb78f7364da53fcdf6964a08c55c92a4b6772bc4560f0cb',
     'gdn.bin':'35329c794e5862bff6c10eb641a586ab6da28d7b6d5029796a4e835e50722bea'}
    for name,digest in pins.items():assert hashlib.sha256((worker/name).read_bytes()).hexdigest()==digest,name
here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='q38-q8-footprint-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_q8_footprint_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*([str(worker)] if worker else [])],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    header=here.parent/'PioneerQ38DenseQ8TokenFootprint.h'
    flags=['-DPIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1']
    subprocess.run([compiler,'-std=c++17',*flags,'-fsyntax-only','-x','c++',str(header)],check=True)
    for flag in flags:
        for replacement in ([],[flag[:-1]+'0']):
            result=subprocess.run([compiler,'-std=c++17',*[f for f in flags if f!=flag],*replacement,
                                   '-fsyntax-only','-x','c++',str(header)],capture_output=True)
            assert result.returncode and b'isolated offline inspection only' in result.stderr,flag
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                 'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_WORK_INFERENCE',
                 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17',*flags,'-D'+role+'=1','-fsyntax-only','-x','c++',str(header)],capture_output=True)
        assert result.returncode and b'isolated offline inspection only' in result.stderr,role
print('PASS dense Q8 footprint role and geometry; NO DEVICE')
