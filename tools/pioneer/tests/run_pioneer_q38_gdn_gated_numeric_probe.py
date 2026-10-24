#!/usr/bin/env python3
"""Original-source CPU/emitted-local diagnostic;NO device or native context."""
import hashlib
import os
import pathlib
import runpy
import struct
import subprocess
import sys
import tempfile

if len(sys.argv) not in (3, 5) or (len(sys.argv)==5 and sys.argv[3]!='--raw-context') or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
model, image = map(pathlib.Path, sys.argv[1:3])
assert model.is_file() and model.stat().st_size == 18973870528
assert hashlib.sha256((image/'code.bin').read_bytes()).hexdigest() == '7ec76ef20a0a453ae24adbda74623b5f95a400ff0f5b15e17e3788f079beb2b8'
assert (image/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qgt-numeric-probe-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    arguments = [str(model), str(image)]
    if len(sys.argv)==5:
        # Already returned CPU bytes only. Strict bounded data parser;NO
        # log text is executed,NO accelerator or native source is created.
        analyzer = runpy.run_path(str(here.parent/'analyze_qgt52_raw_context.py'))
        with pathlib.Path(sys.argv[4]).open('rb') as log:
            data = log.read(1024*1024+1)
        if len(data)>1024*1024:
            raise SystemExit('context log exceeds1MiB')
        context = analyzer['parse'](data.decode('utf-8'))
        analyzer['analyze'](context) # finite NORMAL-or-zero/bounded input gate
        if (context['head'], context['lane'], context['word'], context['expected_bits']) != (47,49,820975,0x3a86371e):
            raise SystemExit('this original-model probe is fixed to the reproduced head47 failure')
        values = [0x31504351,1,47,49,820975,0,context['expected_bits'],context['actual_bits']]
        for name in ('EXPECTED_RAW','ACTUAL_RAW','ORIGINAL_Z'):
            values.extend(context[name])
        assert len(values)==392
        payload = root/'returned-cpu-context.bin'
        payload.write_bytes(struct.pack('<392I',*values))
        arguments.append(str(payload))
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
              '-ffp-contract=off', '-fsanitize=address,undefined']
    bridge = root/'local-emitted-observer.o'
    subprocess.run([*common, '-c', str(here/'pioneer_q38_gdn_gated_norm_bridge.cpp'), '-o', str(bridge)], check=True)
    binary = root/'probe'
    subprocess.run([*common, str(here/'pioneer_q38_gdn_gated_numeric_probe.cpp'), str(bridge), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *arguments], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
