#!/usr/bin/env python3
"""Read-only cache-comparison fixture; no SSH, model or device access."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

path = Path(__file__).resolve().parents[1] / 'q38_haiku_mac_cache_compare.py'
spec = importlib.util.spec_from_file_location('q38_cross_compare', path)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def must_fail(action):
    try:
        action()
    except (ValueError, OSError):
        return
    raise AssertionError('corrupt input accepted')


with tempfile.TemporaryDirectory(prefix='q38-cross-cache-test-') as temp:
    directory = Path(temp)
    label, _, remote, recipe, host, target, _ = mod.SPECS[1]
    fixture_spec = (label, directory, remote, recipe, host, target, 1)
    data = bytes(range(256)) * 300
    parts = [data[i:i + 65536] for i in range(0, len(data), 65536)]
    table = b''.join(hashlib.sha256(part).digest() for part in parts)
    header = bytearray(256)
    struct.pack_into('<8sIIIIQQQ', header, 0, b'P100AWC1', 1, 256, 65536, 0, len(data), len(parts), 42)
    header[48:80] = mod.SOURCE
    header[80:112] = hashlib.sha256((recipe + host).encode()).digest()
    name = 'p100a-weight-v1-' + hashlib.sha256(header).hexdigest() + '.cache'
    header[112:144] = hashlib.sha256(data).digest()
    header[144:176] = hashlib.sha256(table).digest()
    path = directory / name
    original = header + table + data
    path.write_bytes(original)
    entries = mod.collect(fixture_spec)
    assert len(entries) == 1 and entries[0][1] == 42
    good = SimpleNamespace(stdout='0 %s %s\n' % (entries[0][5], entries[0][4]))
    with patch.object(mod.subprocess, 'run', return_value=good):
        assert mod.compare(entries) == 1
    with patch.object(mod.subprocess, 'run', return_value=SimpleNamespace(stdout='0 bad bad\n')):
        must_fail(lambda: mod.compare(entries))
    corrupt = bytearray(original)
    corrupt[-1] ^= 1
    path.write_bytes(corrupt)
    must_fail(lambda: mod.collect(fixture_spec))
    path.write_bytes(original[:-1])
    must_fail(lambda: mod.collect(fixture_spec))
    path.write_bytes(original)
    link = directory / ('p100a-weight-v1-' + '0' * 64 + '.cache')
    link.symlink_to(path)
    must_fail(lambda: mod.collect(fixture_spec))
print('PASS: synthetic cache verification and mismatch, truncation and symlink refusals')
