#!/usr/bin/env python3
"""Execute authenticated old/new whole worker ROMs with synthetic endpoints.

No caller-provided symbol text, emitted opcode relaxation, card IO, speed
extrapolation or native authority. The historical fixture is authenticated
too, before it is replaced by the selected ROM before virgin boot.
"""
import ast
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes

require(len(sys.argv)==3, 'OLD_WIDE NEW_CONSTANT_Q4')
require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role overrides')
here=pathlib.Path(__file__).resolve().parent
control,image=map(pathlib.Path,sys.argv[1:])
mixed=control/'unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'


def pins(script):
    return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
                if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))


def snapshot(directory, records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'absolute ROM directory')
    require(bool(records), 'immutable pins required')
    items=[]
    for name,size,digest in records:
        raw=stable_bytes(directory/name,size)
        require(hashlib.sha256(raw).hexdigest()==digest, 'whole ROM '+name)
        items.append((name,raw))
    return items


snapshots=[snapshot(directory,pins(script)) for directory,script in (
    (mixed,here/'run_pioneer_q38_mixed_worker_tests.py'),
    (control,here.parent/'build_qwen38_wide_gdn_worker_image.py'),
    (image,here.parent/'build_qwen38_q4_constant_issue_worker_image.py'))]
legacy_pins=pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')
legacy_records=[(name+'.bin',size,digest) for name,size,digest in legacy_pins]
legacy_records.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy_snapshot=snapshot(mixed/'unchanged-worker',legacy_records)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q4-constant-execution-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary)
    images=[]
    for i,items in enumerate(snapshots):
        directory=root/('AUTHENTICATED_ROM_'+str(i));directory.mkdir(mode=0o700)
        for name,raw in items:
            path=directory/name
            with path.open('xb') as stream:stream.write(raw)
            path.chmod(0o400)
        (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')]))
        (directory/'program.symbols').chmod(0o400)
        images.append(directory)
    legacy=images[0]/'unchanged-worker';legacy.mkdir(mode=0o700)
    for name,raw in legacy_snapshot:
        path=legacy/name
        with path.open('xb') as stream:stream.write(raw)
        path.chmod(0o400)
    # Independently audit the COMPLETE two emitted helpers: each original
    # command must use the inline rotated encoding; only the exact RV32
    # prologue/epilogue may surround it. In particular, NO MMIO store/call.
    symbols=subprocess.check_output([tool+'nm','-S','--defined-only',str(images[2]/'program.elf')],text=True)
    text=dict(snapshots[2])['text.bin']
    for panel in (0,2):
        rows=[r.split() for r in symbols.splitlines() if '_Z29PioneerQ38Q4ConstantTransformI18VectorPanelEmitterLj'+str(panel)+'EE' in r]
        require(len(rows)==1,'one actual constant helper')
        start,size=int(rows[0][0],16),int(rows[0][1],16)
        raw=text[start-0x72000:start-0x72000+size]
        require(len(raw)==size and size%4==0,'whole actual helper extent')
        commands=[]
        for chunk in range(8):
            block=(chunk//2)*4;parity=(chunk%2)*2
            commands.append(0x700d0000|block|parity)
            commands.append(0x04000000|((2+int(panel==2))<<4)|(3 if chunk==0 else 0))
            if chunk==0:
                if panel==2:commands.append(0x7affc001)
                commands.extend((0x7142000f,0x7e000401))
            commands.append(0x72050000|(128+block+(8 if block>=8 else 0))|parity)
        commands.extend((0x08002040,0x08082044,0x08102048,0x0818204c))
        rotated=[((w<<2)|(w>>30))&0xffffffff for w in commands]
        expected=[0xff010113,0x00812623,0x01010413,*rotated,
                  0x00c12403,0x00100513,0x01010113,0x00008067]
        require(list(struct.unpack('<'+'I'*(len(raw)//4),raw))==expected,'exact inline helper instruction stream')
        print('PASS_ACTUAL_INLINE_CONSTANT_Q4 panel'+str(panel)+' bytes'+str(size)+' commands'+str(len(commands))+' NO_MMIO_COMMAND_STORE',flush=True)
    binary=root/'worker-test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_q4_constant_issue_worker_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*map(str,images)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('CLOSED actual constant-Q4 worker tests;synthetic endpoints only,NO native/model/physical/Work/TPS grant')
