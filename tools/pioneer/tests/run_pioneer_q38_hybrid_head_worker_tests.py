#!/usr/bin/env python3
"""QGW8 worker inspection: role, binary identity and synthetic instruction tests."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==7 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,first,second,old,leaf,oldroot=map(Path,sys.argv[1:]);assert all(p.is_absolute() and p.is_dir() for p in (sdk,first,second,old,leaf,oldroot))
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38HybridMatrixProgramPins import ROOT_PINS,WORKER_PINS
pins={'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
 'globalworker.bin':'5a663114c058168677938681baca39efc5f7e6a3177235c692f37d1d1a064685',
 'bf16producer.bin':'c32a3790c4e5c1387d2a1c6ac2798514d923f7529b2ab073049b02e78d26f8f1',
 'cachedfpu.bin':'310ef63f844703dfb640dd6840af6c5ae5c60107bb349b86eeb13f42158fbbd3',
 'dense.bin':'1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45',
 'text.bin':'262c677556d6b747c8eb1a3fc6a96c34e4c616af8b6c31be8ca2092baa0961d8',
 'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
 'ffn.bin':'ef59bdf258b33c35b7a2571182564a1245b52a8dc541e4e0c99399e220e8c6fe',
 'service.bin':'c4284f945050469db9671d1070b789ee9004ffa04bb4b025a00ffd7f0d72c220',
 'gdn.bin':'35a5dc8cf1c964098a98e953097c283b1a46c81451da454b64fa39af83a9a1e5',
 'program.elf':'d28edbd5cf25b8184d29ddf189b74efe843d934cd8ebb628976ed3331823897d'}
for name,digest in pins.items():
 for folder in (first,second):assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,(folder,name)
for folder,required in ((old,WORKER_PINS),(oldroot,ROOT_PINS)):
 for name,size,digest in required:
  data=(folder/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(folder,name)
for name,digest in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                    ('text.bin','4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')):
 assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
for folder in (first,second):
 assert (folder/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')])
dis=subprocess.check_output([tool+'objdump','-d','-C','-j','.globalworker',str(first/'program.elf')],text=True)
assert '9f442703' in dis and 'lw\ta4,-1548(s0)' in dis # exact pinned simulated-history injection
flags=['-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
 '-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
 '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
role=root/'PioneerQ38HybridHeadWorkerLayout.h';compiler=os.environ.get('CXX','c++')
def check(args,valid):
 r=subprocess.run([compiler,'-std=c++17','-x','c++','-fsyntax-only',*args,str(role)],capture_output=True)
 assert (r.returncode==0)==valid,r.stderr.decode()[:1000]
check(flags,True)
for flag in flags:
 reduced=[v for v in flags if v!=flag];check(reduced,False);check(reduced+[flag[:-1]+'0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_FULL_NATIVE',
 'PIONEER_Q38_HYBRID_MATRIX_NATIVE_127','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
 'PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY','PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0',
 'PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
 check(flags+['-D'+name+'=1'],False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-worker-',dir='/private/tmp') as directory:
 tmp=Path(directory);header=tmp/'bad.h'
 rejected=subprocess.run([sys.executable,str(root/'generate_qwen38_hybrid_programs.py'),
  '--matrix-native-candidate',str(oldroot),str(first),str(header)],capture_output=True)
 assert rejected.returncode and not header.exists()
 compileflags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
 for name in ('layout','image'):
  binary=tmp/name
  subprocess.run([compiler,*compileflags,*(flags if name=='image' else []),
   str(here/('pioneer_q38_hybrid_head_worker_'+name+'_test.cpp')),'-o',str(binary)],check=True)
  subprocess.run([str(binary),*([str(first),str(leaf)] if name=='image' else [])],check=True,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS QGW8 two worker images identical, QGW7 unchanged, role and tag127 generator refuse mixing; synthetic only, no hardware or TPS')
