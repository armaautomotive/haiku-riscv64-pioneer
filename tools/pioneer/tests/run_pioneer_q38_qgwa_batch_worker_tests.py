#!/usr/bin/env python3
"""Test pinned compiled batching worker offline, never access the device."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 6 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_WORKER_A ABS_WORKER_B ABS_BATCH_LEAF ABS_OLD_WORKER ABS_PRIVATE_EXPORT')
first, second, leaf, old, private = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (first, second, leaf, old, private)):
    raise SystemExit('absolute existing directories required')
def pin(path, digest):
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('hash mismatch: '+str(path))
for build in (first, second):
    pin(build/'program.elf', 'e0d7bb5a1434181a8560867f1bfee701a7787a878dff780bce215adf79f2fc47')
    pin(build/'globalworker.bin', '894747f7bf2a282438932a7dc3b56003d9ec64abcda30914b0fd891b70034207')
    pin(build/'cachedfpu.bin', '46b4b2e86bc7b09383a3945ba5241aa8745d6939c0868d73636fe71de0913753')
pin(old/'program.elf', 'd28edbd5cf25b8184d29ddf189b74efe843d934cd8ebb628976ed3331823897d')
pin(leaf/'boot.bin', 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
pin(leaf/'text.bin', '77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')
info = private.lstat()
if not stat.S_ISDIR(info.st_mode) or stat.S_IMODE(info.st_mode) != 0o700 or info.st_uid != os.geteuid():
    raise SystemExit('private directory refused')
private_pins = {'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
                'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
for name, digest in private_pins.items():
    info = (private/name).lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_uid != os.geteuid() or stat.S_IMODE(info.st_mode) != 0o600:
        raise SystemExit('private header refused')
    pin(private/name, digest)
here = Path(__file__).resolve().parent
root = here.parent
compiler = os.environ.get('CXX', 'c++')
names = ('PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION', 'PIONEER_Q38_HYBRID_WORKER_IMAGE',
         'PIONEER_Q38_BF16_WORKER_IMAGE', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT', 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER')
roles = ['-D'+name+'=1' for name in names]
include = '#include "'+str(root/'PioneerQ38Qgw8Bfp8BatchRole.h')+'"\n'
def check(flags, code, expected):
    p = subprocess.run([compiler, '-std=c++17', *flags, '-fsyntax-only', '-x', 'c++', '-'], input=code, text=True, capture_output=True)
    if (p.returncode == 0) != expected:
        raise RuntimeError('role check failed: '+repr(flags)+' '+p.stderr[:1200])
check(roles, include, True)
for flag in roles:
    check([x for x in roles if x != flag], include, flag == roles[-1])
    check([x for x in roles if x != flag]+[flag[:-1]+'0'], include, False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
             'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129','PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
             'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT',
             'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
             'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
             'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
             'PIONEER_Q38_QGW8_BFP8_REUSE_WORKER','PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT',
             'PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(roles+['-D'+name+'=1'], include, False)
check(roles[:-1], include+'#define '+names[-1]+' 1\n'+include, False)
check(roles, include+'#undef '+names[-1]+'\n'+include, False)
loader = '#include "'+str(root/'PioneerQ38Bfp8WorkerLoader.h')+'"\n'
check(roles+['-DPIONEER_Q38_BFP8_WORKER_DIAGNOSTIC=1'], loader, False)
check(roles+['-DPIONEER_Q38_BFP8_WORKER_DIAGNOSTIC=1','-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1'], loader, True)
with tempfile.TemporaryDirectory(prefix='qgwa-test-', dir='/private/tmp') as tmp:
    flags = ['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
             '-fsanitize=address,undefined',*roles,'-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1']
    for source, extra, required in (
        ('pioneer_q38_hybrid_head_worker_image_test.cpp', [], 'PASS actual QGWA'),
        ('pioneer_q38_qgw9_reuse_private_head_test.cpp', ['-I'+str(private)], '576 exact words')):
        binary = Path(tmp)/source
        subprocess.run([compiler,*flags,*extra,str(here/source),'-o',str(binary)],check=True)
        result = subprocess.run([str(binary),str(first),str(leaf)],check=True,capture_output=True,text=True,
                                env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
        if result.stderr or required not in result.stdout: raise RuntimeError('simulation output refused')
        print(result.stdout,end='')
for name, digest in private_pins.items(): pin(private/name, digest)
print('PASS distinct role, two pinned builds, offline simulation; NO hardware or speed proof')
