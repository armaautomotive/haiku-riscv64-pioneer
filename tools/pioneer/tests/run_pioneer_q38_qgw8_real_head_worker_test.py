#!/usr/bin/env python3
"""Build isolated actual-worker simulation; never access the device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) not in (4, 5) or any(not Path(x).is_absolute() for x in sys.argv[1:]):
    raise SystemExit('usage: ABS_QGW8_WORKER_DIR ABS_BFP8_LEAF_DIR ABS_GGUF [ABS_PINNED_CPU_HIDDEN]')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer environment overrides refused')
worker, leaf, model = map(Path, sys.argv[1:4])
pins = {'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
 'globalworker.bin':'5a663114c058168677938681baca39efc5f7e6a3177235c692f37d1d1a064685',
 'bf16producer.bin':'c32a3790c4e5c1387d2a1c6ac2798514d923f7529b2ab073049b02e78d26f8f1',
 'cachedfpu.bin':'310ef63f844703dfb640dd6840af6c5ae5c60107bb349b86eeb13f42158fbbd3',
 'dense.bin':'1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45',
 'text.bin':'262c677556d6b747c8eb1a3fc6a96c34e4c616af8b6c31be8ca2092baa0961d8',
 'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
 'ffn.bin':'ef59bdf258b33c35b7a2571182564a1245b52a8dc541e4e0c99399e220e8c6fe',
 'service.bin':'c4284f945050469db9671d1070b789ee9004ffa04bb4b025a00ffd7f0d72c220',
 'gdn.bin':'35a5dc8cf1c964098a98e953097c283b1a46c81451da454b64fa39af83a9a1e5',
 'program.symbols':'e7da1eb556180fcc34b4003bc5b350e377f6921bc7f11262e53f2aa9f4ba2491'}
for name, digest in pins.items():
    if hashlib.sha256((worker/name).read_bytes()).hexdigest() != digest:
        raise SystemExit('worker pin mismatch: ' + name)
for name, digest in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                     ('text.bin','4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')):
    if hashlib.sha256((leaf/name).read_bytes()).hexdigest() != digest:
        raise SystemExit('leaf pin mismatch: ' + name)
here = Path(__file__).resolve().parent
flags = ['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
         '-fsanitize=address,undefined', '-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1',
         '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-DPIONEER_Q38_BF16_WORKER_IMAGE=1',
         '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
with tempfile.TemporaryDirectory(prefix='qgw8-real-head-worker-',dir='/private/tmp') as tmp:
    exe = Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(here/'pioneer_q38_qgw8_real_head_worker_test.cpp'),
                    str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(exe)],check=True)
    env = {**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'}
    subprocess.run([str(exe),str(worker),str(leaf),str(model),*sys.argv[4:]],env=env,check=True)
