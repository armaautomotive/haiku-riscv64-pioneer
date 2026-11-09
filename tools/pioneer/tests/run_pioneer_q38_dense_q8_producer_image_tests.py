#!/usr/bin/env python3
"""Execute pinned QGWB producer instructions with synthetic history and NoC."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

args=sys.argv[1:];combined='--qgwd' in args
if combined:
    if args.count('--qgwd')!=1:raise SystemExit('duplicate QGWD selector')
    args.remove('--qgwd')
if len(args) not in (4,5) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-SDK absolute-worker1 absolute-worker2 absolute-BFP4-leaf [absolute-BFP8-leaf]')
sdk,first,second,leaf=map(Path,args[:4]);bfp8=Path(args[4]) if len(args)==5 else None
if not all(p.is_absolute() and p.is_dir() for p in (sdk,first,second,leaf,*((bfp8,) if bfp8 else ()))):
    raise SystemExit('absolute existing directories required')
pins={'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
 'bf16producer.bin':'531453b71c59409485087339f36cfd0b48700c7edb53b2b4bcdfd3dea913acd8',
 'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
 'cachedfpu.bin':'b1f1de4a7265ce05f33435c7ab65b5c22b9dbd483ea42febff7361c6cdeed562',
 'dense.bin':'28ea4e30975fea712d82a62ef4be658dc0b3e4ac6cd406c5eeeeb07112750838',
 'ffn.bin':'94ee6dc85ef68047880bc54b1148925ffd30489648ce8c7a6f0acf67649018c7',
 'gdn.bin':'35329c794e5862bff6c10eb641a586ab6da28d7b6d5029796a4e835e50722bea',
 'globalworker.bin':'fa60cd626eba19a49e16f162b1c9f5a9d07aec84f668ad5ea7961950a2e783d1',
 'service.bin':'0456bed771cf6db9185fec31a185a787bcca0ec22c5341523779b94c8da91048',
 'text.bin':'f912ca99337a3032deb78f7364da53fcdf6964a08c55c92a4b6772bc4560f0cb',
 'program.elf':'59d423a1332a80f6aa7adb5e154f7016f619107649ed5a650851856a5fda9fc2'}
if combined:
    pins.update({'globalworker.bin':'cbe9eb61d3d231c254fc0a86f60bbe8b8e89761091fbad12c779e5245c9df85c',
                 'dense.bin':'8d0751a2ca303c122c85bf1952709ff36d8fb0549978f731391fc9ce81845206',
                 'program.elf':'b9ab4753f2e99734cdbeb16b4fc0077688784c8b841a7e7aba128981a610b6f8'})
for folder in (first,second):
    for name,digest in pins.items():
        assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,(folder,name)
    assert len((folder/'globalworker.bin').read_bytes())==13148>12288
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
for name,digest in {'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
                    'text.bin':'04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf'}.items():
    assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
if bfp8:
    for name,digest in {'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
                        'text.bin':'4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67'}.items():
        assert hashlib.sha256((bfp8/name).read_bytes()).hexdigest()==digest,name
dis=subprocess.check_output([tool+'objdump','-d','-C','-j','.globalworker',str(first/'program.elf')],text=True)
assert 'b9042703' in dis and 'lw\ta4,-1136(s0)' in dis,'test-only model-layer injection must match pinned image'
here=Path(__file__).resolve().parent
flags=['-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1',
 '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-DPIONEER_Q38_BF16_WORKER_IMAGE=1',
 '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
if combined:flags+=['-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=1','-DPIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER=1']
if combined:
    header=here.parent/'PioneerQ38DenseQ8MatrixWorkerLayout.h'
    source='#include <cstdint>\n#include "'+str(header)+'"\n'
    def rolecheck(arguments,okay,body=source):
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',*arguments,'-fsyntax-only','-x','c++','-'],
                              input=body,text=True,capture_output=True)
        assert (result.returncode==0)==okay,result.stderr[:1000]
    rolecheck(flags,True)
    for name in ('PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
                 'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
                 'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER'):
        reduced=[flag for flag in flags if flag!='-D'+name+'=1'];rolecheck(reduced,False)
        rolecheck(reduced+['-D'+name+'=0'],False)
    for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_QGW8_BFP8_REUSE_WORKER'):
        rolecheck(flags+['-D'+name+'=1'],False)
    rolecheck(flags,False,source+'#undef PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER\n#include "'+str(header)+'"\n')
with tempfile.TemporaryDirectory(prefix='qgwb-dense-producer-image-',dir='/private/tmp') as directory:
    tests=[('producer',leaf)] + ([('matrix',bfp8),('connected',bfp8)] if bfp8 else [])
    if combined:tests.append(('transition',leaf))
    for name,selected_leaf in tests:
        binary=Path(directory)/name
        source='pioneer_q38_hybrid_matrix_attention_transition_test.cpp' if name=='transition' else 'pioneer_q38_dense_q8_'+name+'_image_test.cpp'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-Wno-missing-field-initializers','-Wno-unused-function','-Wno-unused-variable','-ffp-contract=off',
                        '-fsanitize=address,undefined',*flags,*(['-DPIONEER_TEST_BF16_SRCA_16BIT_BASE=1'] if name=='transition' else []),str(here/source),'-o',str(binary)],check=True)
        for folder in (first,second):
            subprocess.run([str(binary),str(folder),str(selected_leaf)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
print('PASS two pinned '+('QGWD' if combined else 'QGWB')+' producer images; old journal is TOO SMALL; NO NATIVE OR DEVICE AUTHORITY')
