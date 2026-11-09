#!/usr/bin/env python3
"""Session-derived head metadata -> emitted QGW8 ROOT under simulation only."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv) not in (3,5) or (len(sys.argv)==5 and sys.argv[3]!='--qgwa') or any(not Path(x).is_absolute() or not Path(x).is_dir() for x in (sys.argv[1:3]+(sys.argv[4:5] if len(sys.argv)==5 else []))):
    raise SystemExit('usage: run_pioneer_q38_qgw8_held_head_root_bridge.py ABS_HAIKU_BUILD ABS_PINNED_ROOT [--qgwa ABS_PINNED_WORKER]')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer environment overrides refused')
sdk, root_image = map(Path, sys.argv[1:3]);qgwa=len(sys.argv)==5
here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
pins = (('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
        ('text.bin', 129480, '5dc34054cf7615e3176a7575a57548e57db8800ebe528999c098db4955c70820'),
        ('terminal.bin', 8, '6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
        ('conv.bin', 19664, '4d1b37229052436ef0ee6511557863ef8b687829443f2caf65e2afd0f17ac240'),
        ('bf16.bin', 39716, 'a6c558945b9739dc9085b2af3aebe4a895fd3798c55d69c89fea3a30cee3bcd7'),
        ('program.elf', 220156, '8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f'))
if qgwa:
    sys.path.insert(0,str(tools))
    from PioneerQ38QgwaBatchProgramPins import ROOT_PINS
    pins=ROOT_PINS
for name, size, digest in pins:
    data = (root_image / name).read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise RuntimeError('Pinned QGW8 ROOT mismatch: ' + name)
common = [cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
          '-ffp-contract=off', '-fsanitize=address,undefined']
if qgwa:common+=['-DPIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION=1']
sim = ['-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1', '-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1',
       '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1', '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-held-head-root-') as directory:
    tmp = Path(directory)
    image = tmp / 'image'
    image.mkdir()
    for name, _, _ in pins:
        shutil.copyfile(root_image / name, image / name)
    nm = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
    (image / 'program.demangled').write_bytes(subprocess.check_output([nm, '-nC', str(image / 'program.elf')]))
    program_flags=[]
    if qgwa:
        header=tmp/'qgwa-programs.h'
        subprocess.run([sys.executable,str(tools/'generate_qwen38_hybrid_programs.py'),
                        '--qgwa-batch-inspection',str(root_image),sys.argv[4],str(header)],check=True)
        if hashlib.sha256(header.read_bytes()).hexdigest()!='b615fda5f32852814f5383c6ec66c7a80a4c344cd4e7e196a593825beac462d8':
            raise RuntimeError('QGWA header identity mismatch')
        program_flags=['-D'+name+'=1' for name in ('PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION',
            'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC',
            'PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC','PIONEER_Q38_TOKEN_BF16_PROGRAMS')]
        program_flags+=['-I'+str(tools),'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
    session, helpers = tmp / 'session', tmp / 'helpers'
    subprocess.run(common + program_flags + ['-Wno-unused-variable', str(here / 'pioneer_q38_qgw8_upload_session_test.cpp'),
                            str(tools / 'PioneerModelWeightCache.cpp'), '-o', str(session)], check=True)
    subprocess.run(common + sim + [str(here / 'pioneer_q38_hybrid_head_root_helpers_test.cpp'),
                                   '-o', str(helpers)], check=True)
    result = subprocess.run([str(session), '--emit-head-fixture'], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    lines = result.stdout.splitlines()
    if not any(line.startswith('PASS QGW8 actual page router/session/receipt:') for line in lines):
        raise RuntimeError('Full synthetic session did not finish')
    def fixture(label, size):
        matches = [line[len(label) + 1:] for line in lines if line.startswith(label + ' ')]
        if len(matches) != 1:
            raise RuntimeError('Expected one ' + label)
        data = bytes.fromhex(matches[0])
        if len(data) != size:
            raise RuntimeError('Invalid ' + label + ' length')
        return data
    metadata, job = tmp / 'metadata', tmp / 'job'
    metadata.write_bytes(fixture('QGW8_HELD_HEAD_FIXTURE', 1024))
    job.write_bytes(fixture('QGW8_HELD_HEAD_JOB', 256))
    subprocess.run([str(helpers), str(image), str(metadata), str(job)], check=True, env=env)
    receipt = tmp / 'receipt'
    subprocess.run(common + program_flags + ['-Wno-unused-variable', str(here / 'pioneer_q38_qgw8_upload_receipt_test.cpp'),
                             '-o', str(receipt)], check=True)
    result = subprocess.run([str(receipt), '--emit-startup-fixture'], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError('Startup fixture failed: '+result.stderr[-3000:])
    lines = result.stdout.splitlines()
    if not any(line.startswith('PASS QGW8 receipt and actual runtime reservation:') for line in lines):
        raise RuntimeError('Synthetic QGW8 receipt/startup did not finish')
    metadata.write_bytes(fixture('QGW8_STARTUP_HEAD_FIXTURE', 1024))
    job.write_bytes(fixture('QGW8_STARTUP_HEAD_JOB', 256))
    subprocess.run([str(helpers), str(image), str(metadata), str(job)], check=True, env=env)
print('PASS held-session and receipt-derived startup head metadata -> actual linked '+('QGWA' if qgwa else 'QGW8')+' ROOT instructions, synthetic data; no device/inference/TPS')
