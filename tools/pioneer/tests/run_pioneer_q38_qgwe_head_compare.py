#!/usr/bin/env python3
"""Compare QGWD/QGWE emitted head jobs in the same simulated fabric."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

if len(sys.argv)!=5 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_QGWD_WORKER ABS_QGWD_LEAF ABS_QGWE_WORKER ABS_QGWE_LEAF')
old,old_leaf,new,new_leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert len({old,old_leaf,new,new_leaf})==4 and all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (old,old_leaf,new,new_leaf))
for path,digest in ((old/'program.elf','b9ab4753f2e99734cdbeb16b4fc0077688784c8b841a7e7aba128981a610b6f8'),
                    (new/'program.elf','d3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42'),
                    (old_leaf/'text.bin','4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67'),
                    (new_leaf/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for leaf in (old_leaf,new_leaf):
    assert hashlib.sha256((leaf/'boot.bin').read_bytes()).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
base=('PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
      'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
      'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
      'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwe-head-comparison-',dir='/private/tmp') as directory:
    tmp=Path(directory);results=[]
    for name,folder,leaf,extra in (('QGWD',old,old_leaf,()),('QGWE',new,new_leaf,
        ('PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT'))):
        assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
        for section in ('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn'):
            extracted=tmp/(section+'.bin')
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(extracted)],check=True)
            assert extracted.read_bytes()==(folder/(section+'.bin')).read_bytes(),(folder,section)
        binary=tmp/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
            '-ffp-contract=off','-fsanitize=address,undefined',*['-D'+n+'=1' for n in base+extra],
            str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),'-o',str(binary)],check=True)
        result=subprocess.run([str(binary),str(folder),str(leaf)],capture_output=True,text=True,check=True,
            env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
        assert not result.stderr
        matches=re.findall(r'^HEAD_WORKER_SIM bands2 issued_reads(\d+) rv32_steps(\d+);NOT_HARDWARE_TIME$',result.stdout,re.M)
        assert len(matches)==1 and '2048 proof-bit mutations refused' in result.stdout,result.stdout
        reads,steps=map(int,matches[0]);results.append((name,reads,steps))
    assert results[0][1]==640 and results[1][1]==322,results
    for name,reads,steps in results:print(name,'synthetic_reads',reads,'simulated_rv32_steps',steps)
print('SAME synthetic two-band fixture; independent worker outputs checked; NOT hardware cycles, model throughput or speedup')
