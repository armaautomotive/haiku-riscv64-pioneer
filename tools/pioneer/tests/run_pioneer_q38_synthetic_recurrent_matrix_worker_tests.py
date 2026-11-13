#!/usr/bin/env python3
"""Execute one pinned recurrent worker matrix band on synthetic input only."""
import ast
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,worker,leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,worker,leaf))
tree=ast.parse((here.parent/'PioneerQ38RecurrentProgramPins.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
          and any(isinstance(t,ast.Name) and t.id=='WORKER_PINS' for t in n.targets))
for name,size,digest in pins:
    data=(worker/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
assert hashlib.sha256((leaf/'program.elf').read_bytes()).hexdigest()=='e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'
tree=ast.parse((here/'run_pioneer_q38_recurrent_small_dispatch_test.py').read_text())
roles=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
           and any(isinstance(t,ast.Name) and t.id=='roles' for t in n.targets))
cxx=os.environ.get('CXX','c++');tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
with tempfile.TemporaryDirectory(prefix='q38-synthetic-matrix-worker-',dir='/private/tmp') as tmp:
    tmp=Path(tmp);binary=tmp/'test';bridge=tmp/'bridge.o';address=tmp/'address.o';part=tmp/'part.bin'
    assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])==(worker/'program.symbols').read_bytes()
    for folder,sections in ((worker,('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')),
                            (leaf,('boot','text'))):
        for section in sections:
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(part)],check=True)
            assert part.read_bytes()==(folder/(section+'.bin')).read_bytes()
    common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
            '-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined']
    subprocess.run([*common,'-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1','-DPIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST=1','-c',str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',str(bridge)],check=True)
    subprocess.run([*common,'-c',str(here/'pioneer_q38_synthetic_recurrent_address.cpp'),'-o',str(address)],check=True)
    subprocess.run([*common,*['-D'+r+'=1' for r in roles],str(here/'pioneer_q38_synthetic_recurrent_matrix_worker_test.cpp'),str(bridge),str(address),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(worker),str(leaf),str(here/'synthetic_q4_qgw8_layer.fixture')],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for forbidden in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([cxx,'-std=c++17','-D'+forbidden+'=1','-fsyntax-only',
                               str(here/'pioneer_q38_synthetic_recurrent_address.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Synthetic recurrent address observer is host-only' in result.stderr
print('PASS synthetic recurrent worker address role isolation; NO DEVICE')
