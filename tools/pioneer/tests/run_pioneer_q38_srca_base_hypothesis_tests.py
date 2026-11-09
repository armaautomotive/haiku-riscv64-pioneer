#!/usr/bin/env python3
"""Reproduce candidate image and alternative SrcA-base hypothesis tests."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv)!=6 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-SDK absolute-QGWC-A absolute-QGWC-B absolute-QGW8 absolute-BFP4-leaf')
sdk,first,second,old,leaf=map(Path,sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (sdk,first,second,old,leaf)):
    raise SystemExit('absolute existing directories required')
pins={'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
 'globalworker.bin':'f65873029aaaa2493c6eb1ec33297080a4c4292b0a7bf76d0f8252cbf5a42214',
 'dense.bin':'8965216d2e186982204a4f304b62cc7254911e5c463c61b6bb03cea15fc6c810',
 'bf16producer.bin':'c32a3790c4e5c1387d2a1c6ac2798514d923f7529b2ab073049b02e78d26f8f1',
 'cachedfpu.bin':'310ef63f844703dfb640dd6840af6c5ae5c60107bb349b86eeb13f42158fbbd3',
 'text.bin':'262c677556d6b747c8eb1a3fc6a96c34e4c616af8b6c31be8ca2092baa0961d8',
 'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
 'ffn.bin':'ef59bdf258b33c35b7a2571182564a1245b52a8dc541e4e0c99399e220e8c6fe',
 'service.bin':'c4284f945050469db9671d1070b789ee9004ffa04bb4b025a00ffd7f0d72c220',
 'gdn.bin':'35a5dc8cf1c964098a98e953097c283b1a46c81451da454b64fa39af83a9a1e5',
 'program.elf':'3450e5b2d46b27e9ac3f81cb62de79d53b0613d85af0acc03a73a149e812e09e'}
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
for folder in (first,second):
    for name,digest in pins.items():assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,(folder,name)
    assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
assert hashlib.sha256((old/'globalworker.bin').read_bytes()).hexdigest()=='5a663114c058168677938681baca39efc5f7e6a3177235c692f37d1d1a064685'
assert hashlib.sha256((old/'dense.bin').read_bytes()).hexdigest()=='1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45'
previous=(old/'dense.bin').read_bytes();candidate=(first/'dense.bin').read_bytes()
assert len(previous)==len(candidate)==14468 and [(i,a,b) for i,(a,b) in enumerate(zip(previous,candidate)) if a!=b]==[(8647,8,4),(13615,8,4)]
for name,digest in {'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
                    'text.bin':'04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf'}.items():
    assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
        '-Wno-missing-field-initializers','-Wno-unused-function','-Wno-unused-variable','-ffp-contract=off','-fsanitize=address,undefined']
candidate_flag='-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=1';model='-DPIONEER_TEST_BF16_SRCA_16BIT_BASE=1'
worker_flags=[candidate_flag,model,'-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
              '-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
              '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
header=here.parent/'PioneerQ38DenseBf16Matrix.h'
for flags in ([candidate_flag], [candidate_flag,'-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY=1','-DPIONEER_Q38_WORK_INFERENCE=1'],
              ['-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=0','-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY=1']):
    result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',*flags,'-fsyntax-only','-x','c++',str(header)],capture_output=True)
    assert result.returncode,flags
head=here.parent/'PioneerQ38HybridHeadWorkerLayout.h';headflags=[f for f in worker_flags if f!=model]
subprocess.run([compiler,'-std=c++17','-include','cstdint',*headflags,'-fsyntax-only','-x','c++',str(head)],check=True)
for forbidden in ('PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
                  'PIONEER_Q38_QGW8_BFP8_REUSE_WORKER','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION',
                  'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE'):
    result=subprocess.run([compiler,'-std=c++17','-include','cstdint',*headflags,'-D'+forbidden+'=1',
                           '-fsyntax-only','-x','c++',str(head)],capture_output=True)
    assert result.returncode,forbidden
source='#include "PioneerQ38HybridHeadWorkerLayout.h"\n#undef PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE\n#include "PioneerQ38HybridHeadWorkerLayout.h"\n'
result=subprocess.run([compiler,'-std=c++17','-include','cstdint','-I',str(here.parent),*headflags,
                       '-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
assert result.returncode and 'late QGWC candidate removal' in result.stderr
with tempfile.TemporaryDirectory(prefix='q38-srca-hypothesis-',dir='/private/tmp') as directory:
    tmp=Path(directory);env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1',ASAN_OPTIONS='quarantine_size_mb=32')
    for name,source,flags in (
      ('model','pioneer_q38_dense_bf16_srca_base_hypothesis_test.cpp',[]),
      ('numeric','pioneer_q38_dense_bf16_matrix_test.cpp',[candidate_flag,model,'-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY=1']),
      ('transition','pioneer_q38_hybrid_matrix_attention_transition_test.cpp',worker_flags)):
        binary=tmp/name;subprocess.run([*common,*flags,str(here/source),'-o',str(binary)],check=True)
        for folder in ((first,second) if name=='transition' else (None,)):
            subprocess.run([str(binary),*([str(folder),str(leaf)] if folder else [])],check=True,env=env)
print('PASS QGWC two pinned worker images, two candidate instruction changes and alternative-model tests; NO SILICON OR SPEED CLAIM')
