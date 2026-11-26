#!/usr/bin/env python3
"""Check the offline qualification image package and its compile guards."""

import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import tempfile

script = Path(__file__).resolve().parents[1] / 'build_qwen35_qualification_service_header.py'
spec = importlib.util.spec_from_file_location('qualification_header', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as temporary:
    image = Path(temporary)
    boot = bytes.fromhex('6fc00600')
    code = bytes.fromhex('130000006f0000001300000013000000')
    dispatch = bytes.fromhex('1300000013000000')
    symbols = '''00072000 T qwen35_dataflow_entry
00072004 T qwen35_dataflow_stopped_park
00072008 T qwen35_dataflow_service_main
0007200c T qwen35_shared_compute_coordinate
000a0400 T qwen35_graph_execute
'''
    for name, data in (('boot.bin', boot), ('text.bin', code), ('dispatch.bin', dispatch),
                       ('symbols.txt', symbols.encode())):
        (image / name).write_bytes(data)
    for marker in ('activation-sharing.marker', 'fused-recurrence.marker'):
        (image / marker).touch()
    header = module.generate(image)
    for marker in ('dense-shard.marker', 'ttsim-unprofiled.marker', 'flash-hybrid.marker', 'ffn-pair.marker', 'fused-rms-save.marker'):
        (image / marker).touch()
        try:
            module.generate(image)
            raise AssertionError('accepted non-packageable ' + marker)
        except ValueError:
            pass
        finally:
            (image / marker).unlink()
    assert 'DataflowQualificationServiceStoppedParkAddress=0x00072004U' in header
    assert 'DataflowQualificationServiceBootWord=0x0006c06fU' in header
    for name, expected in (('DataflowQualificationServiceTextWords', code),
                           ('DataflowQualificationServiceDispatchWords', dispatch)):
        body = header.split(name + '[]={\n', 1)[1].split('};', 1)[0]
        assert b''.join(struct.pack('<I', int(word.strip().removesuffix('U,'), 16))
                        for word in body.splitlines()) == expected

    def rejected(name, replacement):
        path = image / name
        original = path.read_bytes()
        path.write_bytes(replacement)
        try:
            try:
                module.generate(image)
            except ValueError:
                return
            raise AssertionError('accepted invalid ' + name)
        finally:
            path.write_bytes(original)

    rejected('boot.bin', b'\0' * 4)
    rejected('text.bin', b'\0' * len(code))
    rejected('dispatch.bin', b'')
    rejected('dispatch.bin', b'\0' * (0x5c00 + 4))
    rejected('symbols.txt', symbols.replace('00072004', '00072002').encode())
    rejected('symbols.txt', symbols.replace('000a0400', '000a0408').encode())
    rejected('symbols.txt', (symbols + '00072000 T qwen35_dataflow_entry\n').encode())
    rejected('symbols.txt', (symbols + '00072000 T qwen35_resident_graph_service_main\n').encode())
    for marker in ('activation-sharing.marker', 'fused-recurrence.marker'):
        (image / marker).unlink()
        try:
            try:
                module.generate(image)
            except ValueError:
                pass
            else:
                raise AssertionError('accepted missing ' + marker)
        finally:
            (image / marker).touch()

    generated = image / 'qualification.h'
    generated.write_text(header, encoding='ascii')
    source = image / 'guard.cpp'
    source.write_text('#include "qualification.h"\n', encoding='ascii')
    command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', str(source)]
    required = ['PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS', 'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                'PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS', 'PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS',
                'PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS']
    def compiles(defines):
        return subprocess.run(command + ['-D' + define for define in defines],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    assert compiles(required)
    for absent in required:
        assert not compiles([define for define in required if define != absent])
    for forbidden in ('PIONEER_QWEN35_RECOVERY_QUALIFY', 'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL'):
        assert not compiles(required + [forbidden])
    assert not compiles(required + ['PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY'])
    (image / 'delta-shard.marker').touch()
    generated.write_text(module.generate(image), encoding='ascii')
    assert compiles(required + ['PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY'])
    assert not compiles(required)

print('PASS qualification image exact packaging and compile guards; OFFLINE ONLY')
