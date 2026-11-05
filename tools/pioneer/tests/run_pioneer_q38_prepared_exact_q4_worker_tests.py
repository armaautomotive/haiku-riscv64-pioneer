#!/usr/bin/env python3
"""Closed actual-worker ASan/UBSan tests;synthetic endpoints,NO device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes
from pioneer_q38_prepared_q4_relocations import audit

require(__debug__ and not os.environ.get('PYTHONOPTIMIZE'),'no optimized Python')
two_product_replay='--two-product-replay' in sys.argv[1:]
two_product='--two-product' in sys.argv[1:] or two_product_replay
if two_product_replay:
    require(sys.argv.count('--two-product-replay')==1 and '--two-product' not in sys.argv,'one explicit replay13 variant,not two modes')
    sys.argv.remove('--two-product-replay')
elif two_product:
    require(sys.argv.count('--two-product')==1,'one explicit two-product CPU validation selector')
    sys.argv.remove('--two-product')
require(len(sys.argv) in (2,3),'IMAGE [--pilot|--group-pilot|--controls-only|--faults-only|--history-only]')
require(len(sys.argv)!=3 or sys.argv[2] in ('--pilot','--group-pilot','--controls-only','--faults-only','--history-only'),'explicit test scope')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'no role override')
here=pathlib.Path(__file__).resolve().parent
image=pathlib.Path(sys.argv[1]);control=image/('unchanged-prepared/unchanged-original' if two_product else 'unchanged-original')
fixture=control/'unchanged-vector/unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
environment={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off',
        '-Wall','-Wextra','-Werror','-fsanitize=address,undefined']


def pins(script,variable='PINS'):
    return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
                if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==variable for t in n.targets))


def snapshot(directory, records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(),'absolute real ROM directory')
    require(bool(records),'whole immutable ROM pins')
    result=[]
    for name,size,digest in records:
        data=stable_bytes(directory/name,size)
        require(hashlib.sha256(data).hexdigest()==digest,'whole ROM '+name)
        result.append((name,data))
    return result


snapshots=[snapshot(directory,pins(script,variable)) for directory,script,variable in (
    (fixture,here/'run_pioneer_q38_mixed_worker_tests.py','PINS'),
    (control,here.parent/'build_qwen38_exact_q4_work_worker.py','PINS'),
    (image,here.parent/('build_qwen38_two_product_q4_worker.py' if two_product else 'build_qwen38_prepared_exact_q4_worker.py'),'REPLAY_PINS' if two_product_replay else 'PINS'))]
legacy=[(name+'.bin',size,digest) for name,size,digest in pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy=snapshot(fixture/'unchanged-worker',legacy)
with tempfile.TemporaryDirectory(prefix='pioneer-prepared-q4-closed-tests-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);images=[]
    for i,items in enumerate(snapshots):
        directory=root/('AUTHENTICATED_ROM_'+str(i));directory.mkdir(mode=0o700)
        for name,data in items:
            target=directory/name
            with target.open('xb') as stream:stream.write(data)
            target.chmod(0o400)
        (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')]))
        (directory/'program.symbols').chmod(0o400);images.append(directory)
    original=images[0]/'unchanged-worker';original.mkdir(mode=0o700)
    for name,data in legacy:
        target=original/name
        with target.open('xb') as stream:stream.write(data)
        target.chmod(0o400)
    audit(images[1],images[2],tool,two_product=two_product)
    data_test=root/'prepared-data'
    subprocess.run([*common,str(here/'pioneer_q38_prepared_q4_data_test.cpp'),'-o',str(data_test)],check=True)
    subprocess.run([str(data_test)],check=True,env=environment)
    flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
    if two_product:
        flags.append('-DPIONEER_Q38_TWO_PRODUCT_WORKER_TEST=1')
        print('TWO_PRODUCT_CLOSED_WORKER_CPU_VALIDATION_ONLY_NO_DEVICE',flush=True)
    if two_product_replay:
        flags.append('-DPIONEER_Q38_TWO_PRODUCT_REPLAY_WORKER_TEST=1')
        print('REPLAY13_SHARED_PHYSICAL_BUFFER_CLOSED_WORKER_VALIDATION_ONLY',flush=True)
    history_only=len(sys.argv)==3 and sys.argv[2]=='--history-only'
    if not history_only:
        binary=root/'worker'
        subprocess.run([*common,*flags,str(here/'pioneer_q38_prepared_exact_q4_worker_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),*map(str,images),*sys.argv[2:]],check=True,env=environment)
    if len(sys.argv)==2 or history_only:
        history=root/'gdn-history'
        subprocess.run([*common,*flags,str(here/'pioneer_q38_prepared_q4_gdn_history_test.cpp'),'-o',str(history)],check=True)
        subprocess.run([str(history),str(images[0]),str(images[2])],check=True,env=environment)
print(('REPLAY13_TWO_PRODUCT_' if two_product_replay else 'TWO_PRODUCT_' if two_product else '')+'CLOSED_PREPARED_Q4_WORKER_TESTS whole immutable ROMs;NO native/ROOT/source/card/Assistant/full-model/TPS grant',flush=True)
