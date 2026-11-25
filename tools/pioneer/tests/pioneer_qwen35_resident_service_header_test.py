#!/usr/bin/env python3
"""Check exact packaging and failure-closed image validation without a card."""

import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import tempfile


script = Path(__file__).resolve().parents[1] / 'build_qwen35_resident_service_header.py'
spec = importlib.util.spec_from_file_location('resident_header', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as temporary:
    image = Path(temporary)
    boot = bytes.fromhex('6fc00600')
    code = bytes.fromhex('130000006f0000001300000013000000')
    dispatch = bytes.fromhex('1300000013000000')
    symbols = '''00072000 T qwen35_dataflow_entry
00072004 T qwen35_resident_graph_stopped_park
00072008 T qwen35_resident_graph_service_main
0007200c T qwen35_shared_compute_coordinate
000a0400 T qwen35_graph_run_descriptor
000a0404 T qwen35_graph_execute
'''
    (image / 'boot.bin').write_bytes(boot)
    (image / 'text.bin').write_bytes(code)
    (image / 'dispatch.bin').write_bytes(dispatch)
    (image / 'symbols.txt').write_text(symbols, encoding='ascii')
    header = module.generate(image)
    assert 'DataflowServiceStoppedParkAddress=0x00072004U' in header
    assert 'DataflowServiceImageBootWord=0x0006c06fU' in header
    assert 'PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS' in header
    for name, expected in (('DataflowServiceTextWords', code),
                           ('DataflowServiceDispatchWords', dispatch)):
        body = header.split(name + '[]={\n', 1)[1].split('};', 1)[0]
        actual = b''.join(struct.pack('<I', int(word.strip().removesuffix('U,'), 16))
                          for word in body.splitlines())
        assert actual == expected

    def rejected(file, replacement):
        path = image / file
        original = path.read_bytes()
        path.write_bytes(replacement)
        try:
            try:
                module.generate(image)
            except ValueError:
                return
            raise AssertionError('accepted invalid ' + file)
        finally:
            path.write_bytes(original)

    rejected('boot.bin', b'\0' * 4)
    rejected('text.bin', b'\0' * len(code))
    rejected('dispatch.bin', b'')
    (image / 'dispatch.bin').write_bytes(b'\0' * 0x6000)
    module.generate(image)
    (image / 'dispatch.bin').write_bytes(dispatch)
    rejected('dispatch.bin', b'\0' * (0x6000 + 4))
    rejected('symbols.txt', symbols.replace('00072004', '00072002').encode())
    rejected('symbols.txt', symbols.replace('000a0404', '000a0408').encode())
    rejected('symbols.txt', (symbols + '00072000 T qwen35_dataflow_entry\n').encode())

    generated = image / 'resident.h'
    generated.write_text(header, encoding='ascii')
    source = image / 'guard-test.cpp'
    source.write_text('#include "resident.h"\n', encoding='ascii')
    command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', str(source)]
    required = ['PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS', 'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                'PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
                'PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS']
    def compiles(defines):
        return subprocess.run(command + ['-D' + define for define in defines],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    assert compiles(required)
    for absent in required:
        assert not compiles([define for define in required if define != absent])
    for forbidden in ('PIONEER_QWEN35_RECOVERY_QUALIFY', 'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL'):
        assert not compiles(required + [forbidden])

print('PASS resident image exact packaging and rejected invalid layout; OFFLINE ONLY')
