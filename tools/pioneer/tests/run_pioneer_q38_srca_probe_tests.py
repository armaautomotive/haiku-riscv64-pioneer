#!/usr/bin/env python3
"""Build and simulate the distinct SrcA probe; never access the device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 4 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-SDK absolute-existing-QGW8 absolute-existing-BFP4-leaf')
sdk, worker, leaf = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (sdk, worker, leaf)):
    raise SystemExit('absolute existing directories required')
for path, digest in ((worker/'globalworker.bin', '5a663114c058168677938681baca39efc5f7e6a3177235c692f37d1d1a064685'),
                     (worker/'dense.bin', '1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45'),
                     (leaf/'boot.bin', 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                     (leaf/'text.bin', '04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf')):
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, path
here = Path(__file__).resolve().parent
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-Wno-misleading-indentation', '-Wno-missing-field-initializers', '-Wno-unused-function',
          '-Wno-unused-variable', '-ffp-contract=off', '-fsanitize=address,undefined',
          '-DPIONEER_TEST_BF16_SRCA_16BIT_BASE=1']
flags = ['-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1', '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
         '-DPIONEER_Q38_BF16_WORKER_IMAGE=1', '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
         '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
env = dict(os.environ, UBSAN_OPTIONS='halt_on_error=1', ASAN_OPTIONS='quarantine_size_mb=32')
lease = here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerNativeBfpLease.h'
for forbidden_flags in ([], ['-DPIONEER_NATIVE_SRCA_PROBE_OWNER_TEST_ONLY=1', '-D_KERNEL_MODE=1'],
              ['-DPIONEER_NATIVE_SRCA_PROBE_OWNER_TEST_ONLY=1', '-DPIONEER_Q38_WORK_INFERENCE=1'],
              ['-DPIONEER_NATIVE_SRCA_PROBE_OWNER_TEST_ONLY=1', '-DPIONEER_DRIVER_BUILD_TAG=1']):
    result = subprocess.run([common[0], '-std=c++17', '-DB_DEVICE_OP_CODES_END=10000',
                             '-DPIONEER_NATIVE_SRCA_PROBE_DIAGNOSTIC=1', *forbidden_flags,
                             '-fsyntax-only', '-x', 'c++', str(lease)], capture_output=True, text=True)
    assert result.returncode and 'confined to offline owner tests' in result.stderr, forbidden_flags
with tempfile.TemporaryDirectory(prefix='q38-srca-probe-', dir='/private/tmp') as directory:
    tmp = Path(directory)
    for name in ('a', 'b'):
        subprocess.run([sys.executable, str(here.parent/'build_q38_srca_probe.py'), str(sdk), str(tmp/name)], check=True)
    for name, digest in {'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
                         'text.bin':'3a816460d7edb6339169f9d626ea5394e040cbb9677884f8fdcef1e8ae553762'}.items():
        for image in ('a', 'b'):
            assert hashlib.sha256((tmp/image/name).read_bytes()).hexdigest() == digest
    for name, source, extra in (('control', 'pioneer_q38_srca_probe_test.cpp', []),
                                ('image', 'pioneer_q38_srca_probe_image_test.cpp', flags),
                                ('owner', 'pioneer_q38_srca_probe_owner_test.cpp', flags)):
        binary = tmp/name
        subprocess.run([*common, *extra, str(here/source), '-o', str(binary)], check=True)
        for image in (('a', 'b') if name in ('image', 'owner') else (None,)):
            args = [str(worker), str(leaf), str(tmp/image)] if image else []
            subprocess.run([str(binary), *args], check=True, env=env)
    subprocess.run([sys.executable, str(here/'pioneer_native_bfp_mapping_test.py'), '--srca'], check=True, env=env)
    subprocess.run([sys.executable, str(here/'pioneer_native_bfp_client_test.py'), '--srca'], check=True, env=env)
print('PASS pinned duplicate images and synthetic tests; NO SILICON OR PERFORMANCE CLAIM')
