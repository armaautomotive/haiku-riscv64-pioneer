#!/usr/bin/env python3
"""Local arithmetic+raw-lane tests;optionally execute authenticated worker ROMs."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes

require(len(sys.argv) in (1,3), 'optional OLD_WIDE NEW_VECTOR_METADATA')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ), 'no ambient role override')
here = pathlib.Path(__file__).resolve().parent
environment = {**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
common = [os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off',
          '-Wall','-Wextra','-Werror','-fsanitize=address,undefined']


def pins(script):
    return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
                if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))


def snapshot(directory,records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'absolute ROM directory')
    require(bool(records), 'immutable complete ROM pins required')
    result = []
    for name,size,digest in records:
        raw = stable_bytes(directory/name,size)
        require(hashlib.sha256(raw).hexdigest()==digest, 'whole authenticated ROM '+name)
        result.append((name,raw))
    return result


with tempfile.TemporaryDirectory(prefix='pioneer-vector-q4-tests-',dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('metadata','block'):
        binary = root/name
        subprocess.run([*common,str(here/('pioneer_q38_vector_q4_'+name+'_test.cpp')),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env=environment)
    if len(sys.argv)==3:
        control,image = map(pathlib.Path,sys.argv[1:])
        mixed = control/'unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
        snapshots = [snapshot(directory,pins(script)) for directory,script in (
            (mixed,here/'run_pioneer_q38_mixed_worker_tests.py'),
            (control,here.parent/'build_qwen38_wide_gdn_worker_image.py'),
            (image,here.parent/'build_qwen38_vector_q4_metadata_worker.py'))]
        legacy = [(name+'.bin',size,digest) for name,size,digest in pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
        legacy.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
        legacy_snapshot = snapshot(mixed/'unchanged-worker',legacy)
        tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
        images = []
        for i,items in enumerate(snapshots):
            directory = root/('AUTHENTICATED_ROM_'+str(i));directory.mkdir(mode=0o700)
            for name,raw in items:
                target = directory/name
                with target.open('xb') as stream:
                    stream.write(raw)
                target.chmod(0o400)
            (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')]))
            (directory/'program.symbols').chmod(0o400)
            images.append(directory)
        old = images[0]/'unchanged-worker';old.mkdir(mode=0o700)
        for name,raw in legacy_snapshot:
            target = old/name
            with target.open('xb') as stream:
                stream.write(raw)
            target.chmod(0o400)
        flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
        binary = root/'worker'
        subprocess.run([*common,*flags,str(here/'pioneer_q38_vector_q4_worker_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),*map(str,images)],check=True,env=environment)
print('CLOSED_VECTOR_Q4_TESTS local only;NO native/card/Assistant/model-quality/TPS/speedup grant',flush=True)
