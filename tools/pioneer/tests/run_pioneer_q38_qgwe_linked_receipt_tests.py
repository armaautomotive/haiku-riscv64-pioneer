#!/usr/bin/env python3
"""Run the linked QGWE controller receipt on mocked QGWE worker proofs."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 5 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_QGWE_WORKER ABS_BATCH_LEAF ABS_QGWE_ROOT ABS_QGWD_ROOT')
worker, leaf, root, old = map(Path, sys.argv[1:])
if len({worker,leaf,root,old}) != 4 or not all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (worker,leaf,root,old)):
    raise SystemExit('four distinct absolute directories required')
def pin(path, digest):
    assert not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest, path
for folder,digest in ((worker,'d3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42'),
                      (root,'2aed2c3bd11821fbe9317aaefcab98f849a8f5adadebeef8f8365a43173edfe4'),
                      (old,'2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95')):
    pin(folder/'program.elf',digest)
pin(leaf/'boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
pin(leaf/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
roles=('PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
       'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwe-linked-receipt-',dir='/private/tmp') as tmp:
    temp=Path(tmp)
    for folder,sections in ((worker,('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn')),
                            (root,('text','bf16','conv','terminal')),(old,('text','bf16','conv','terminal'))):
        assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
        for section in sections:
            out=temp/(section+'.bin')
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(out)],check=True)
            assert out.read_bytes()==(folder/(section+'.bin')).read_bytes(),(folder,section)
    binary=temp/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',*['-D'+n+'=1' for n in roles],
                    str(Path(__file__).resolve().parent/'pioneer_q38_qgwa_linked_root_receipt_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(worker),str(leaf),str(root),str(old)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS linked QGWE receipt with simulated worker; old QGWD refuses; NO full ROOT, device, quality or speed')
