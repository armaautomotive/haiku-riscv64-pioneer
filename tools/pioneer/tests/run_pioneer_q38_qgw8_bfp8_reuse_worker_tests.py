#!/usr/bin/env python3
"""Actual QGW9 reuse-worker instructions in simulation, never hardware."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 7 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_REUSE_WORKER_A ABS_REUSE_WORKER_B ABS_OLD_WORKER ABS_REUSE_LEAF ABS_OLD_LEAF ABS_PRIVATE_HEAD_EXPORT')
first, second, old, leaf, old_leaf, private = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (first, second, old, leaf, old_leaf, private)):
    raise SystemExit('absolute build directories required')
info = private.lstat()
if not stat.S_ISDIR(info.st_mode) or stat.S_IMODE(info.st_mode)!=0o700 or info.st_uid!=os.geteuid():
    raise SystemExit('private export directory refused')
def pin(path, expected):
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected: raise RuntimeError('image pin mismatch: '+str(path))
for folder in (first, second):
    pin(folder/'program.elf', '937bd1036260828ca8b3c7b02a8e3911a421f928b215a946a87f621e20103293')
    pin(folder/'globalworker.bin', '1851514eeb417f177c748cdf8c7b157292b88c439527a6c45f469b863f92f622')
    pin(folder/'cachedfpu.bin', '01658a5008c9f07f869d7488df328359f518b967947a3b6c7652447a474df31c')
pin(old/'program.elf', 'd28edbd5cf25b8184d29ddf189b74efe843d934cd8ebb628976ed3331823897d')
pin(leaf/'boot.bin', 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
pin(leaf/'text.bin', '76341ce62314cb654d47e49e9fa2d50232ce9400d6ad55a1aa2aca64b987f7dc')
pin(old_leaf/'boot.bin', 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
pin(old_leaf/'text.bin', '4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')
private_pins = {'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
                'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
for name, digest in private_pins.items():
    info = (private/name).lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1 or info.st_uid!=os.geteuid() or stat.S_IMODE(info.st_mode)!=0o600:
        raise SystemExit('private header identity refused')
    pin(private/name, digest)
here = Path(__file__).resolve().parent; root = here.parent; compiler = os.environ.get('CXX', 'c++')
names = ('PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION', 'PIONEER_Q38_HYBRID_WORKER_IMAGE',
         'PIONEER_Q38_BF16_WORKER_IMAGE', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT', 'PIONEER_Q38_QGW8_BFP8_REUSE_WORKER')
roles = ['-D'+name+'=1' for name in names]
role_header = root/'PioneerQ38Qgw8Bfp8ReuseRole.h'
def check(flags, code, valid):
    result = subprocess.run([compiler, '-std=c++17', *flags, '-fsyntax-only', '-x', 'c++', '-'],
                            input=code, text=True, capture_output=True)
    if (result.returncode == 0) != valid: raise RuntimeError((flags, result.stderr[:1500]))
include = '#include "'+str(role_header)+'"\n'
check(roles, include, True)
for flag in roles:
    check([x for x in roles if x != flag], include, flag.endswith('QGW8_BFP8_REUSE_WORKER=1'))
    check([x for x in roles if x != flag]+[flag[:-1]+'0'], include, False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_QGW8_HIDDEN_NATIVE_129',
             'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
             'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT',
             'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
             'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
             'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
             'PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION','PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(roles+['-D'+name+'=1'], include, False)
check(roles[:-1], include+'#define PIONEER_Q38_QGW8_BFP8_REUSE_WORKER 1\n'+include, False)
check(roles, include+'#undef PIONEER_Q38_QGW8_BFP8_REUSE_WORKER\n'+include, False)
loader = '#include "'+str(root/'PioneerQ38Bfp8WorkerLoader.h')+'"\n'
check(roles+['-DPIONEER_Q38_BFP8_WORKER_DIAGNOSTIC=1'], loader, False)
check(roles+['-DPIONEER_Q38_BFP8_WORKER_DIAGNOSTIC=1','-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1'], loader, True)
with tempfile.TemporaryDirectory(prefix='qgw9-reuse-worker-test-', dir='/private/tmp') as tmp:
    binary = Path(tmp)/'image-test'
    flags = ['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
             '-fsanitize=address,undefined',*roles,'-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1']
    subprocess.run([compiler,*flags,str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),'-o',str(binary)],check=True)
    result = subprocess.run([str(binary),str(first),str(leaf)],check=True,capture_output=True,text=True,
                            env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    if result.stderr or 'mutated retained input silently changes result' not in result.stdout or 'PASS actual QGW9' not in result.stdout:
        raise RuntimeError('expected simulation/fault-control output missing')
    print(result.stdout, end='')
    old_binary = Path(tmp)/'old-image-test'
    old_flags = [f for f in flags if f not in ('-DPIONEER_Q38_QGW8_BFP8_REUSE_WORKER=1',
                                                 '-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1')]
    subprocess.run([compiler,*old_flags,str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),'-o',str(old_binary)],check=True)
    result = subprocess.run([str(old_binary),str(old),str(old_leaf)],check=True,capture_output=True,text=True,
                            env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    if result.stderr or 'PASS actual QGW8' not in result.stdout: raise RuntimeError('old image regression')
    print(result.stdout, end='')
    private_binary = Path(tmp)/'private-image-test'
    subprocess.run([compiler,*flags,'-I'+str(private),str(here/'pioneer_q38_qgw9_reuse_private_head_test.cpp'),'-o',str(private_binary)],check=True)
    result = subprocess.run([str(private_binary),str(first),str(leaf)],check=True,capture_output=True,text=True,
                            env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    if result.stderr or '576 exact words' not in result.stdout: raise RuntimeError('private real-head simulation failed')
    print(result.stdout, end='')
for name, digest in private_pins.items(): pin(private/name, digest)
print('PASS distinct QGW9 role and pinned builds; synthetic simulation only, NOT device or ownership proof')
