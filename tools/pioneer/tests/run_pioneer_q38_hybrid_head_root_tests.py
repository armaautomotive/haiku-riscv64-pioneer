#!/usr/bin/env python3
"""Pinned QGW8 ROOT instruction checks; no device or native authority."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==7 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,first,second,old,worker,leaf=map(Path,sys.argv[1:]);assert all(p.is_absolute() and p.is_dir() for p in (sdk,first,second,old,worker,leaf))
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38HybridMatrixProgramPins import ROOT_PINS
pins=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',129480,'5dc34054cf7615e3176a7575a57548e57db8800ebe528999c098db4955c70820'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',19664,'4d1b37229052436ef0ee6511557863ef8b687829443f2caf65e2afd0f17ac240'),
 ('bf16.bin',39716,'a6c558945b9739dc9085b2af3aebe4a895fd3798c55d69c89fea3a30cee3bcd7'),
 ('program.elf',220156,'8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f'))
for folder,expected in ((first,pins),(second,pins),(old,ROOT_PINS)):
 for name,size,digest in expected:
  data=(folder/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(folder,name)
workerPins={'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
 'globalworker.bin':'5a663114c058168677938681baca39efc5f7e6a3177235c692f37d1d1a064685',
 'bf16producer.bin':'c32a3790c4e5c1387d2a1c6ac2798514d923f7529b2ab073049b02e78d26f8f1',
 'cachedfpu.bin':'310ef63f844703dfb640dd6840af6c5ae5c60107bb349b86eeb13f42158fbbd3',
 'dense.bin':'1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45',
 'text.bin':'262c677556d6b747c8eb1a3fc6a96c34e4c616af8b6c31be8ca2092baa0961d8',
 'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
 'ffn.bin':'ef59bdf258b33c35b7a2571182564a1245b52a8dc541e4e0c99399e220e8c6fe',
 'service.bin':'c4284f945050469db9671d1070b789ee9004ffa04bb4b025a00ffd7f0d72c220',
 'gdn.bin':'35a5dc8cf1c964098a98e953097c283b1a46c81451da454b64fa39af83a9a1e5',
 'program.elf':'d28edbd5cf25b8184d29ddf189b74efe843d934cd8ebb628976ed3331823897d',
 'program.symbols':'e7da1eb556180fcc34b4003bc5b350e377f6921bc7f11262e53f2aa9f4ba2491'}
for name,digest in workerPins.items():assert hashlib.sha256((worker/name).read_bytes()).hexdigest()==digest,name
for name,digest in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                    ('text.bin','4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')):
 assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');compiler=os.environ.get('CXX','c++')
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined']
sim=['-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1',
 '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
rootRoles=['-DPIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION=1','-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1',
 '-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1','-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
 '-DPIONEER_Q38_GLOBAL_CHAIN_BF16_FPU=1','-DPIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP=1',
 '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
def rolecheck(args,valid):
 r=subprocess.run([tool+'g++','-std=c++11','-ffreestanding','-march=rv32imf','-mabi=ilp32','-fsyntax-only',
  *args,str(here/'pioneer_q38_hybrid_head_root_role_probe.cpp')],capture_output=True)
 assert (r.returncode==0)==valid,r.stderr.decode()[:1000]
rolecheck(rootRoles,True)
for flag in rootRoles:
 reduced=[v for v in rootRoles if v!=flag];rolecheck(reduced,False);rolecheck(reduced+[flag[:-1]+'0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_FULL_NATIVE',
 'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
 rolecheck(rootRoles+['-D'+name+'=1'],False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-root-',dir='/private/tmp') as directory:
 tmp=Path(directory);image=tmp/'image';image.mkdir()
 for extra in (['--head-matrix-inspection'],['--head-matrix-inspection','--hybrid-matrix-attention','--hybrid-queued-counters']):
  target=tmp/'rejected';r=subprocess.run([sys.executable,str(root/'build_qwen38_token_root.py'),str(sdk),str(target),*extra],capture_output=True)
  assert r.returncode and not target.exists()
 forbidden=tmp/'old-native-header'
 r=subprocess.run([sys.executable,str(root/'generate_qwen38_hybrid_programs.py'),
  '--matrix-native-candidate',str(first),str(worker),str(forbidden)],capture_output=True)
 assert r.returncode and not forbidden.exists()
 for name,_,_ in pins:shutil.copyfile(first/name,image/name)
 (image/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(image/'program.elf')]))
 plan=tmp/'plan';job=tmp/'job';manifest=tmp/'manifest';metadata=tmp/'metadata';actual=tmp/'actual';helpers=tmp/'helpers';peer=tmp/'peer';proof=tmp/'proof';gather=tmp/'gather'
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_hybrid_head_plan_test.cpp'),'-o',str(plan)],check=True)
 subprocess.run([compiler,*flags,*sim,str(here/'pioneer_q38_hybrid_head_actual_root_test.cpp'),'-o',str(actual)],check=True)
 subprocess.run([compiler,*flags,*sim,str(here/'pioneer_q38_hybrid_head_root_helpers_test.cpp'),'-o',str(helpers)],check=True)
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_hybrid_head_gather_test.cpp'),'-o',str(gather)],check=True)
 workerRoles=['-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
  '-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
 subprocess.run([compiler,*flags,*workerRoles,str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),'-o',str(peer)],check=True)
 env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
 subprocess.run([str(plan),str(job),str(manifest),str(metadata)],check=True,env=env)
 subprocess.run([str(peer),str(worker),str(leaf),str(job),str(proof)],check=True,env=env)
 subprocess.run([str(actual),str(image),str(manifest)],check=True,env=env)
 subprocess.run([str(helpers),str(image),str(metadata),str(job),str(proof)],check=True,env=env)
 subprocess.run([str(gather),str(image),str(metadata),str(proof)],check=True,env=env)
print('PASS QGW8 ROOT independently reproduced, QGW7 ROOT unchanged; actual returning ROOT job instructions, synthetic data only; NO hardware or TPS')
