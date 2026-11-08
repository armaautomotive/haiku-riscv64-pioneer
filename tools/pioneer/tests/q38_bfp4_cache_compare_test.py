#!/usr/bin/env python3
"""Synthetic integrity and mismatch tests; no model, SSH or accelerator."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile

path = Path(__file__).resolve().parents[1] / 'q38_bfp4_cache_compare.py'
spec = importlib.util.spec_from_file_location('q38_compare', path)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def fail(call):
    try:
        call()
    except (ValueError, OSError):
        return
    raise AssertionError('invalid input accepted')


def fixture(root):
    (root / '.p100a-weights-v1').write_bytes(mod.MARKER)
    data = bytes(range(256)) * 300
    chunks = [data[i:i + mod.CHUNK] for i in range(0, len(data), mod.CHUNK)]
    table = b''.join(hashlib.sha256(chunk).digest() for chunk in chunks)
    header = bytearray(256)
    struct.pack_into('<8sIIIIQQQ', header, 0, b'P100AWC1', 1, 256, mod.CHUNK, 0,
                     len(data), len(chunks), 12345)
    header[48:80] = bytes.fromhex(mod.SOURCE)
    header[80:112] = hashlib.sha256((mod.RECIPE + mod.HOST).encode()).digest()
    filename = 'p100a-weight-v1-' + hashlib.sha256(header).hexdigest() + '.cache'
    header[112:144] = hashlib.sha256(data).digest()
    header[144:176] = hashlib.sha256(table).digest()
    result = root / filename
    result.write_bytes(header + table + data)
    return result


with tempfile.TemporaryDirectory(prefix='q38-bfp4-comparison-test-') as temp:
    root = Path(temp)
    entry = fixture(root)
    assert len(mod.inventory(str(root), mod.HOST, 1)) == 1
    fail(lambda: mod.inventory(str(root), mod.TARGET, 1))
    fail(lambda: mod.inventory(str(root), mod.HOST, 2))
    original = entry.read_bytes()
    corrupted = bytearray(original)
    corrupted[-1] ^= 1
    entry.write_bytes(corrupted)
    fail(lambda: mod.inventory(str(root), mod.HOST, 1))
    entry.write_bytes(original[:-1])
    fail(lambda: mod.inventory(str(root), mod.HOST, 1))
    entry.write_bytes(original)
    wrong = root / ('p100a-weight-v1-' + '0' * 64 + '.cache')
    entry.rename(wrong)
    fail(lambda: mod.inventory(str(root), mod.HOST, 1))
    wrong.rename(entry)
    stored = root / 'fixture-data'
    entry.rename(stored)
    entry.symlink_to(stored)
    fail(lambda: mod.inventory(str(root), mod.HOST, 1))

target = {'%016x:100' % i: '%064x' % i for i in range(1071)}
host = dict(target)
for layer in (48, 49):
    for i in range(21):
        host['%016x:100' % ((layer << 48) | i)] = '%064x' % i
assert '1071 matched' in mod.compare(host, target)
wrong_layers = dict(host)
del wrong_layers['%016x:100' % ((49 << 48) | 0)]
wrong_layers['%016x:100' % ((48 << 48) | 21)] = '0' * 64
fail(lambda: mod.compare(wrong_layers, target))
missing = dict(host)
del missing['%016x:100' % 7]
fail(lambda: mod.compare(missing, target))
host['%016x:100' % 5] = 'changed'
fail(lambda: mod.compare(host, target))
print('PASS: synthetic cache integrity and cross-platform mismatch refusals')
