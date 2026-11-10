#!/usr/bin/env python3
"""Check QGWD startup head metadata against the actual linked RV32 ROOT."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
args=sys.argv[1:]; seal_loop='--qgwd-seal-loop' in args
if seal_loop:
    assert args.count('--qgwd-seal-loop')==1; args.remove('--qgwd-seal-loop')
if len(args) != 3:
    raise SystemExit('usage: run_pioneer_q38_qgwd_startup_head_tests.py [--qgwd-seal-loop] ABS_HAIKU_BUILD ABS_QGWD_ROOT ABS_QGWD_HEADER')
sdk, root_image, header = map(Path, args)
assert all(p.is_absolute() for p in (sdk, root_image, header)) and sdk.is_dir() and root_image.is_dir() and header.is_file()
for path, digest in ((root_image/'program.elf', '2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95' if seal_loop else '5cf33c2c4eaf010879acd03a99cdd166c931e0a8400cc082322d407fe841aa21'),
                     (header, '184f10919fb20dd04489a64115022f924cf81c270c318c7da2e0cf78491fc328')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest, path
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
sys.path.insert(0,str(root))
from PioneerQ38QgwdDenseProgramPins import ROOT_PINS
if seal_loop:
    changed={'text.bin':(131028,'ebbe7c9f13615671f48042b833920a22a173dc09b3cc1d5a0548d7bb72af474b'),
             'bf16.bin':(48336,'61f4cf7f3ad3cbb1dcdd8292fd068195fa272c36e9b868c170f2471489f2c558'),
             'program.elf':(229060,'2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95')}
    ROOT_PINS=[(name,*changed[name]) if name in changed else (name,size,digest) for name,size,digest in ROOT_PINS]
for name,size,digest in ROOT_PINS:
    payload=(root_image/name).read_bytes()
    assert len(payload)==size and hashlib.sha256(payload).hexdigest()==digest,name
common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
        '-ffp-contract=off','-fsanitize=address,undefined','-I'+str(root)]
flags=['-DPIONEER_Q38_QGWD_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWD_PLAN_SIMULATION=1',
       '-DPIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE=1','-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
       '-DPIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION=1','-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1',
       '-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1','-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',
       '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
simulation=['-DPIONEER_Q38_QGWD_PLAN_SIMULATION=1','-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
            '-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
            '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='qgwd-head-root-',dir='/private/tmp') as directory:
    tmp=Path(directory);receipt=tmp/'receipt';helpers=tmp/'helpers';probe=tmp/'root';probe.mkdir()
    subprocess.run([*common,*flags,str(here/'pioneer_q38_qgw8_upload_receipt_test.cpp'),'-o',str(receipt)],check=True)
    result=subprocess.run([str(receipt),'--emit-startup-fixture'],text=True,capture_output=True,env=env)
    if result.returncode:
        raise RuntimeError('synthetic startup fixture failed: '+result.stderr[:2000])
    data={}
    for label,size in (('QGW8_STARTUP_HEAD_FIXTURE',1024),('QGW8_STARTUP_HEAD_JOB',256)):
        found=re.findall(r'^'+label+r' ([0-9a-f]+)$',result.stdout,re.M);assert len(found)==1
        payload=bytes.fromhex(found[0]);assert len(payload)==size;data[label]=tmp/label;data[label].write_bytes(payload)
    assert int.from_bytes(data['QGW8_STARTUP_HEAD_FIXTURE'].read_bytes()[(64+40)*4:(64+41)*4],'little')==0x44574751
    assert int.from_bytes(data['QGW8_STARTUP_HEAD_JOB'].read_bytes()[:4],'little')==0x44574751
    for name in ('bf16.bin','text.bin','terminal.bin','conv.bin'):(probe/name).write_bytes((root_image/name).read_bytes())
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    (probe/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(root_image/'program.elf')]))
    subprocess.run([*common,*simulation,str(here/'pioneer_q38_hybrid_head_root_helpers_test.cpp'),'-o',str(helpers)],check=True)
    subprocess.run([str(helpers),str(probe),str(data['QGW8_STARTUP_HEAD_FIXTURE']),str(data['QGW8_STARTUP_HEAD_JOB'])],check=True,env=env)
print('PASS '+('QGWD_SEAL_LOOP' if seal_loop else 'QGWD')+' startup head and actual linked ROOT agree; original pinned synthetic fixture, NO device or inference')
