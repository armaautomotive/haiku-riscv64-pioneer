#!/usr/bin/env python3
"""Incomplete native-layer DATA tests;NEVER device IO or real model results."""
import os
import pathlib
import runpy
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 1 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    for options in ([], ['--native-test-bridge']):
        sys.argv = [str(here/'run_pioneer_q38_observed_trace_tests.py'), *options]
        runpy.run_path(sys.argv[0], run_name='__main__')
    sys.argv = [str(here/'run_pioneer_q38_observed_trace_role_tests.py')]
    runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
flags = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined']
environment = {**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='pioneer-q38-native-layer-data-', dir='/private/tmp') as temporary:
    temporary = pathlib.Path(temporary)
    for facade in (False, True):
        extra = (['-DPIONEER_Q38_NATIVE_TRACE_FACADE_TEST=1', str(here/'pioneer_q38_native_trace_test_bridge.cpp')]
                 if facade else [])
        name = 'facade' if facade else 'direct'
        out, prefix, faults = (temporary/(name+suffix) for suffix in ('-out', '-prefix', '-faults'))
        for path in (out, prefix, faults):
            path.mkdir(mode=0o700)
        binary = temporary/(name+'-positive')
        subprocess.run([*flags, '-DPIONEER_Q38_NATIVE_LAYER_SNAPSHOT_DATA_TEST=1', *extra,
                        str(here/'pioneer_q38_observed_trace_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary), str(out), str(prefix)], check=True, env=environment)
        snapshots = sorted(out.glob('INCOMPLETE-native-position-*-layers-*.bin'))
        assert len(snapshots) == 192
        for position in range(3):
            token = (out/f'token-{position}.bin').read_bytes()
            assert len(token) == (16+65*5120+248320+64+1)*4
            for layers in range(1, 65):
                data = (out/f'INCOMPLETE-native-position-{position}-layers-{layers}.bin').read_bytes()
                assert len(data) == (16+5120+1)*4
                assert struct.unpack_from('<16I', data) == (0x314c4e51, 1, position, 248319 if position == 0 else 17,
                                                          layers, 5120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
                assert data[-4:] == struct.pack('<I', 0x454e444c)
                assert struct.unpack_from('<5120f', data, 64) == tuple(float(position)+(i%13)*.125 for i in range(5120))
                assert data[64:-4] == token[(16+layers*5120)*4:(16+(layers+1)*5120)*4]
        binary = temporary/(name+'-negative')
        subprocess.run([*flags, *extra, str(here/'pioneer_q38_native_layer_snapshot_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary), str(faults)], check=True, env=environment)
        print(f'PASS {name}192 SYNTHETIC layer snapshots retain exact complete token DATA/independent headers,lengths,footers,ALL words;'
              '20 refusals;NO observed model arithmetic,terminal,END,physical,Work or timing proof', flush=True)
