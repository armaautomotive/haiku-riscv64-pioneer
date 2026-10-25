#!/usr/bin/env python3
"""Original-source LOCAL precision counterfactual;NO device/native IO."""
import hashlib
import os
import pathlib
import runpy
import struct
import subprocess
import sys
import tempfile

if len(sys.argv) != 7 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
model, legacy, precise, prepare, gated, log = map(pathlib.Path, sys.argv[1:])
assert model.is_file() and model.stat().st_size == 18973870528
pins = ((legacy, 'd4b6f84cfbeec3abc63a7c130245bd557966ef0412c2f46a93d5159589b6891d'),
        (precise, 'c9d917403ae39611d262b1f97aa0bf44e23c21661fec33b6f9a3739493ef08f6'),
        (prepare, '746e4a3600ed18d8710c311883dd0833802dcbbe37b2dac7c6be48f6dfd53894'),
        (gated, '7ec76ef20a0a453ae24adbda74623b5f95a400ff0f5b15e17e3788f079beb2b8'))
for directory, pin in pins:
    assert hashlib.sha256((directory/'code.bin').read_bytes()).hexdigest() == pin
    assert (directory/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
here = pathlib.Path(__file__).resolve().parent
analysis = runpy.run_path(str(here.parent/'analyze_qgt52_prepared_context.py'))
with log.open('rb') as stream:
    data = stream.read(1024*1024+1)
assert len(data) <= 1024*1024
context = analysis['parse'](data.decode('utf-8'))
analysis['analyze'](context)
assert (context['head'], context['lane'], context['word'], context['expected_bits']) == (47,49,820975,0x3a86371e)
payload = [0x31505343,1,47,49,820975,context['expected_bits'],context['actual_bits'],0]
for name in ('EXPECTED_PREPARED_0','EXPECTED_PREPARED_1','ACTUAL_PREPARED_0','ACTUAL_PREPARED_1',
             'EXPECTED_RAW','ACTUAL_RAW','ORIGINAL_Z','EXPECTED_NEXT_ROW','ACTUAL_NEXT_ROW'):
    payload.extend(context[name])
assert len(payload) == 1936
with tempfile.TemporaryDirectory(prefix='pioneer-silu-original-probe-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    captured = root/'captured-cpu-data.bin';captured.write_bytes(struct.pack('<1936I',*payload))
    common = [os.environ.get('CXX','c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
              '-ffp-contract=off', '-fsanitize=address,undefined']
    objects = []
    for name in ('pioneer_q38_silu_precision_probe_bridge','pioneer_q38_unscaled_prepare_probe_bridge','pioneer_q38_gdn_gated_norm_bridge'):
        target = root/(name+'.o')
        subprocess.run([*common, '-c', str(here/(name+'.cpp')), '-o', str(target)], check=True)
        objects.append(str(target))
    binary = root/'probe'
    subprocess.run([*common,str(here/'pioneer_q38_silu_precision_original_probe.cpp'),*objects,'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(model),str(legacy),str(precise),str(prepare),str(gated),str(captured)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
