#!/usr/bin/env python3
"""Recorder format/lifecycle tests with synthetic observations, NEVER model IO."""
import os
import pathlib
import struct
import subprocess
import sys
import tempfile
here = pathlib.Path(__file__).resolve().parent
assert len(sys.argv) == 1 or sys.argv[1:] == ['--native-test-bridge']
extra = ['-DPIONEER_Q38_NATIVE_TRACE_FACADE_TEST=1', str(here/'pioneer_q38_native_trace_test_bridge.cpp')] if len(sys.argv) == 2 else []
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='pioneer-observed-trace-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    output = pathlib.Path(tmp)/'output'
    output.mkdir(mode=0o700)
    prefix = pathlib.Path(tmp)/'prefix'
    prefix.mkdir(mode=0o700)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    *extra, str(here/'pioneer_q38_observed_trace_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(output), str(prefix)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    for position in range(3):
        data = (output/f'token-{position}.bin').read_bytes()
        assert len(data) == (16+65*5120+248320+64+1)*4
        h = struct.unpack_from('<16I', data)
        assert h == (0x31444f51, 1, position, 248319 if position == 0 else 17, 17, 0, 0x3f800000, 0, 65, 5120, 248320, 64, 0, 0, 0, 0)
        assert data[-4:] == bytes.fromhex('4f444e45')
    assert sorted(p.name for p in prefix.iterdir()) == ['prefix-0.bin']
    data = (prefix/'prefix-0.bin').read_bytes()
    assert len(data) == (16+2*5120+1)*4
    assert struct.unpack_from('<16I', data) == (0x31504f51, 1, 0, 248319, 2, 5120, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    assert data[-4:] == bytes.fromhex('50444e45')
    print('PASS independent Python observed format/header/length/footer checks;not model correctness')
