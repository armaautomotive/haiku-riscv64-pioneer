#!/usr/bin/env python3
"""Exercise both pinned QGWF worker formats against mocked NoC/Tensix."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

if len(sys.argv) not in (5,6) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_WORKER_A ABS_WORKER_B ABS_BFP4_BATCH_LEAF ABS_BFP8_BATCH_LEAF [ABS_PRIVATE_BANDS]')
first,second,four,eight=map(Path,sys.argv[1:5]);private=Path(sys.argv[5]) if len(sys.argv)==6 else None
here=Path(__file__).resolve().parent
assert len({first,second,four,eight})==4 and all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (first,second,four,eight))
for path,digest in ((first/'program.elf','c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e'),
                    (second/'program.elf','c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e'),
                    (four/'text.bin','2942f4c427b17592d3149f3fcaf5cc197f64570702033189390749a851da51c9'),
                    (eight/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for folder in (four,eight):assert hashlib.sha256((folder/'boot.bin').read_bytes()).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
private_pins=('edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd',
              '59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695',
              '036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3')
if private:
    info=private.lstat()
    assert private.is_absolute() and stat.S_ISDIR(info.st_mode) and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o700
    for index,digest in enumerate(private_pins):
        path=private/('band'+str(index)+'.rbp4');info=path.lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
        assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
names=('PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER',
       'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
       'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
       'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwf-worker-test-',dir='/private/tmp') as directory:
    tmp=Path(directory)
    for folder in (first,second):
        assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
        for section in ('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn'):
            out=tmp/(section+'.bin');subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(out)],check=True)
            assert out.read_bytes()==(folder/(section+'.bin')).read_bytes(),section
    for label,source,leaf,extra,expected in (
        ('bfp4','pioneer_q38_hybrid_worker_image_test.cpp',four,(), 'dispatcher -> BFP4 leaf'),
        ('bfp8','pioneer_q38_hybrid_head_worker_image_test.cpp',eight,('PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT',),'PASS QGWF')):
        binary=tmp/label
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-ffp-contract=off','-fsanitize=address,undefined',*['-D'+name+'=1' for name in names+extra],
                        str(here/source),'-o',str(binary)],check=True)
        for folder in (first,second):
            result=subprocess.run([str(binary),str(folder),str(leaf)],capture_output=True,text=True,check=True,
                env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
            assert not result.stderr and expected in result.stdout,result.stdout+result.stderr
            print(result.stdout,end='')
    if private:
        binary=tmp/'private-bfp4'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-ffp-contract=off','-fsanitize=address,undefined',*['-D'+name+'=1' for name in names],
                        str(here/'pioneer_q38_qgwf_private_bfp4_test.cpp'),'-o',str(binary)],check=True)
        for folder in (first,second):
            for index in range(3):
                result=subprocess.run([str(binary),str(folder),str(four),str(private/('band'+str(index)+'.rbp4'))],
                    capture_output=True,text=True,check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
                assert not result.stderr and '192 exact words' in result.stdout,result.stdout+result.stderr
                print(result.stdout,end='')
print('PASS pinned dual-batch worker on synthetic BFP4/BFP8 jobs and old-identity refusals; NO DEVICE OR TPS')
