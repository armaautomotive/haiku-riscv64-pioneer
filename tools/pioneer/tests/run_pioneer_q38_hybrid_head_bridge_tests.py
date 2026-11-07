#!/usr/bin/env python3
"""Actual generated head job -> actual QGW8 RV32 worker; synthetic fabric."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
worker,leaf=map(Path,sys.argv[1:]);assert all(p.is_absolute() and p.is_dir() for p in (worker,leaf))
here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
expected={'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
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
for name,digest in expected.items():assert hashlib.sha256((worker/name).read_bytes()).hexdigest()==digest,name
for name,digest in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                    ('text.bin','4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')):
 assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
roles=['-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
 '-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
 '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-bridge-',dir='/private/tmp') as name:
 tmp=Path(name);plan=tmp/'plan';image=tmp/'image';fixture=tmp/'job'
 subprocess.run([compiler,*flags,'-Wno-unused-function',str(here/'pioneer_q38_hybrid_head_plan_test.cpp'),'-o',str(plan)],check=True)
 subprocess.run([compiler,*flags,*roles,str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),'-o',str(image)],check=True)
 env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
 subprocess.run([str(plan),str(fixture)],check=True,env=env);assert fixture.stat().st_size==256
 subprocess.run([str(image),str(worker),str(leaf),str(fixture)],check=True,env=env)
print('PASS actual full-coverage planner job -> pinned QGW8/BFP8 emitted worker; injected previous-layer state and synthetic operands, no real model/device/TPS')
