#!/usr/bin/env python3
"""QGWF linked ROOT receipt with real emitted worker, mocked fabric only."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=6 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_WORKER ABS_BFP4_LEAF ABS_BFP8_LEAF ABS_QGWF_ROOT ABS_QGWE_ROOT')
worker,four,eight,root,old=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert len({worker,four,eight,root,old})==5 and all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (worker,four,eight,root,old))
for path,digest in ((worker/'program.elf','c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e'),
                    (root/'program.elf','5257f8da3b692ae964a0f29e7d895d0274985c113a8137653c53c655b34d8454'),
                    (old/'program.elf','a10e4e5c0971d94b3a60e1a730a5d58f3aeccea67ceacd4c076903a9d554505e'),
                    (four/'text.bin','2942f4c427b17592d3149f3fcaf5cc197f64570702033189390749a851da51c9'),
                    (eight/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for leaf in (four,eight):assert hashlib.sha256((leaf/'boot.bin').read_bytes()).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
names=('PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER',
       'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
       'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwf-linked-receipt-',dir='/private/tmp') as directory:
    tmp=Path(directory)
    for folder,sections in ((worker,('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn')),
                            (root,('text','bf16','conv','terminal')),(old,('text','bf16','conv','terminal'))):
        assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
        for section in sections:
            out=tmp/(section+'.bin');subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(out)],check=True)
            assert out.read_bytes()==(folder/(section+'.bin')).read_bytes(),(folder,section)
    symbols=subprocess.check_output([tool+'nm','-nC',str(root/'program.elf')],text=True)
    relocated=[int(line.split()[0],16) for line in symbols.splitlines() if 'PioneerQ38HybridWorkerLayout::LeafShape<PioneerQ38Bfp4WorkerJob>' in line]
    assert len(relocated)==3 and all(0x20000<=a<0x30000 for a in relocated),relocated
    for label,source,leaf,extra in (('bfp4','pioneer_q38_qgwf_linked_bfp4_receipt_test.cpp',four,()),
                                    ('bfp8','pioneer_q38_qgwa_linked_root_receipt_test.cpp',eight,('PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT',))):
        binary=tmp/label
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-ffp-contract=off','-fsanitize=address,undefined',*['-D'+n+'=1' for n in names+extra],str(here/source),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(worker),str(leaf),str(root),str(old)],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS actual QGWF ROOT/worker receipts for BFP4 and BFP8 on mocked fabric; NOT full model, device or speed')
