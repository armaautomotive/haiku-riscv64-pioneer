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
args=sys.argv[1:];qgwf='--qgwf' in args
if qgwf:
    assert args.count('--qgwf')==1;args.remove('--qgwf');args.append('--qgwe')
qgwe='--qgwe' in args
if qgwe:
    assert args.count('--qgwe')==1;args.remove('--qgwe')
seal_loop='--qgwd-seal-loop' in args
assert not (qgwe and seal_loop)
if seal_loop:
    assert args.count('--qgwd-seal-loop')==1; args.remove('--qgwd-seal-loop')
if len(args) not in ((3,5) if qgwe else (3,)):
    raise SystemExit('usage: startup_head_tests.py [--qgwe|--qgwd-seal-loop] ABS_SDK ABS_ROOT ABS_HEADER [ABS_QGWE_WORKER ABS_BFP8_LEAF]')
sdk, root_image, header = map(Path, args[:3])
worker,leaf=map(Path,args[3:]) if len(args)==5 else (None,None)
assert all(p.is_absolute() for p in (sdk, root_image, header)) and sdk.is_dir() and root_image.is_dir() and header.is_file()
for path, digest in ((root_image/'program.elf', '5257f8da3b692ae964a0f29e7d895d0274985c113a8137653c53c655b34d8454' if qgwf else 'a10e4e5c0971d94b3a60e1a730a5d58f3aeccea67ceacd4c076903a9d554505e' if qgwe else '2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95' if seal_loop else '5cf33c2c4eaf010879acd03a99cdd166c931e0a8400cc082322d407fe841aa21'),
                     (header, 'e0931f355b9eee374e8b18e0127fd84c21d9a9ef39a1252fedf3371e6e56ac0d' if qgwf else '5097fc1b4aa43bf1a9a1418345e138649293c8e2ee2cea7097b92989a7ba14bf' if qgwe else '184f10919fb20dd04489a64115022f924cf81c270c318c7da2e0cf78491fc328')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest, path
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
sys.path.insert(0,str(root))
if qgwf:from PioneerQ38QgwfDualBatchProgramPins import ROOT_PINS
elif qgwe:from PioneerQ38QgweBatchDenseProgramPins import ROOT_PINS
else:from PioneerQ38QgwdDenseProgramPins import ROOT_PINS
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
if qgwe:
    flags+=['-DPIONEER_Q38_QGWE_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWE_BATCH_DENSE_PLAN=1']
    simulation+=['-DPIONEER_Q38_QGWE_BATCH_DENSE_PLAN=1']
if qgwf:
    flags+=['-DPIONEER_Q38_QGWF_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWF_DUAL_BATCH_PLAN=1']
    simulation+=['-DPIONEER_Q38_QGWF_DUAL_BATCH_PLAN=1']
env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='qgwd-head-root-',dir='/private/tmp') as directory:
    tmp=Path(directory);receipt=tmp/'receipt';helpers=tmp/'helpers';probe=tmp/'root';probe.mkdir()
    subprocess.run([*common,*flags,str(here/'pioneer_q38_qgw8_upload_receipt_test.cpp'),'-o',str(receipt)],check=True)
    result=subprocess.run([str(receipt),'--emit-startup-fixture'],text=True,capture_output=True,env=env)
    if result.returncode:
        raise RuntimeError('synthetic startup fixture failed: '+result.stderr[:2000])
    if worker:
        assert worker.is_absolute() and leaf.is_absolute() and worker.is_dir() and leaf.is_dir()
        for path,digest in ((worker/'program.elf','c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e' if qgwf else 'd3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42'),
                            (leaf/'boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                            (leaf/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')):
            assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
        tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
        assert (worker/'program.symbols').read_bytes()==subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])
        for section in ('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn'):
            extracted=tmp/(section+'.bin')
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(worker/'program.elf'),str(extracted)],check=True)
            assert extracted.read_bytes()==(worker/(section+'.bin')).read_bytes(),section
        records=[]
        for number in range(1,120):
            found=re.findall(r'^QGWE_STARTUP_WORKER_'+str(number)+r' ([0-9a-f]+)$',result.stdout,re.M)
            assert len(found)==1 and len(found[0])==512,number;records.append(bytes.fromhex(found[0]))
        fixture=tmp/'worker-boots.bin';fixture.write_bytes(b''.join(records));boots=tmp/'worker-boots'
        worker_flags=['-D'+name+'=1' for name in ('PIONEER_Q38_QGWE_BATCH_DENSE_WORKER',
            'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER',
            'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
            'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
            'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
            'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT')]
        if qgwf:worker_flags+=['-DPIONEER_Q38_QGWF_BATCH4_WORKER=1','-DPIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT=1']
        subprocess.run([*common,*worker_flags,str(here/'pioneer_q38_qgwe_all_worker_boot_test.cpp'),'-o',str(boots)],check=True)
        subprocess.run([str(boots),str(worker),str(leaf),str(fixture)],check=True,env=env)
    data={}
    for label,size in (('QGW8_STARTUP_HEAD_FIXTURE',1024),('QGW8_STARTUP_HEAD_JOB',256)):
        found=re.findall(r'^'+label+r' ([0-9a-f]+)$',result.stdout,re.M);assert len(found)==1
        payload=bytes.fromhex(found[0]);assert len(payload)==size;data[label]=tmp/label;data[label].write_bytes(payload)
    witness=0x46574751 if qgwf else 0x45574751 if qgwe else 0x44574751
    assert int.from_bytes(data['QGW8_STARTUP_HEAD_FIXTURE'].read_bytes()[(64+40)*4:(64+41)*4],'little')==witness
    assert int.from_bytes(data['QGW8_STARTUP_HEAD_JOB'].read_bytes()[:4],'little')==witness
    for name in ('bf16.bin','text.bin','terminal.bin','conv.bin'):(probe/name).write_bytes((root_image/name).read_bytes())
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    (probe/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(root_image/'program.elf')]))
    subprocess.run([*common,*simulation,str(here/'pioneer_q38_hybrid_head_root_helpers_test.cpp'),'-o',str(helpers)],check=True)
    subprocess.run([str(helpers),str(probe),str(data['QGW8_STARTUP_HEAD_FIXTURE']),str(data['QGW8_STARTUP_HEAD_JOB'])],check=True,env=env)
print('PASS '+('QGWF' if qgwf else 'QGWE' if qgwe else 'QGWD_SEAL_LOOP' if seal_loop else 'QGWD')+' startup head and actual linked ROOT agree; synthetic fixture, NO device or inference')
