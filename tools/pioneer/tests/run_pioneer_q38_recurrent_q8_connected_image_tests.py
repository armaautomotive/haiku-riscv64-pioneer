#!/usr/bin/env python3
"""Simulate the compiled recurrent worker with synthetic input and weights."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv)!=4 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_SDK ABS_WORKER_IMAGE ABS_BFP8_LEAF')
sdk,worker,leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,worker,leaf))
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
names=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWF_BATCH4_WORKER',
       'PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT',
       'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='q38-recurrent-connected-',dir='/private/tmp') as directory:
    tmp=Path(directory)
    assert (worker/'program.symbols').read_bytes()==subprocess.check_output([str(tool)+'-nm','-n',str(worker/'program.elf')])
    for section in ('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn'):
        out=tmp/(section+'.bin')
        subprocess.run([str(tool)+'-objcopy','-O','binary','-j','.'+section,str(worker/'program.elf'),str(out)],check=True)
        assert out.read_bytes()==(worker/(section+'.bin')).read_bytes(),section
    binary=tmp/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off',
                    '-fsanitize=address,undefined',*['-D'+name+'=1' for name in names],
                    str(here/'pioneer_q38_recurrent_q8_connected_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(worker),str(leaf)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
