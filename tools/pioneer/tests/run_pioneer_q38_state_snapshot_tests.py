#!/usr/bin/env python3
"""Synthetic snapshot normalization/format/refusal tests, never model/card IO."""
import importlib.util
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
spec = importlib.util.spec_from_file_location('state_compare', here/'compare_pioneer_q38_state_snapshots.py')
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-state-component-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    cpu, native, refused, legacy = (base/name for name in ('cpu', 'native', 'refused', 'legacy'))
    for path in (cpu, native, refused, legacy):
        path.mkdir(mode=0o700)
    binary = base/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_native_state_snapshot_test.cpp'),
                    str(here/'pioneer_q38_native_state_test_bridge.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(cpu), str(native), str(refused), str(legacy)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    assert not any(refused.iterdir())
    manifest = lambda directory: {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in directory.iterdir()}
    legacy_before = manifest(legacy)
    compare.compare(cpu, native)
    compare.compare(cpu, legacy, legacy_raw=True)
    assert manifest(legacy) == legacy_before, 'legacy decoder rewrote DATA artifacts'
    rejected = 0
    def reject(operation=None):
        global rejected
        try:
            (operation or (lambda: compare.compare(cpu, native)))()
        except (AssertionError, OSError):
            rejected += 1
        else:
            raise AssertionError('corrupted/missing corpus accepted')
    # These are synthetic fixture outputs in a private TemporaryDirectory,
    # not model files or evidence. Restore each mutation before the next.
    path = native/'c3-l63-k3.bin'
    original = path.read_bytes()
    for offset, value in ((0, compare.CPU_MAGIC), (4, 3), (8, 2),
                          (12, 62), (16, 2), (20, 1535), (24, 1), (64, 0x7c000000),
                          (64, 0x00007c00), (len(original)-4, 0)):
        changed = bytearray(original)
        struct.pack_into('<I', changed, offset, value)
        path.write_bytes(changed)
        reject(lambda: compare.read(native, path.name, compare.NATIVE_MAGIC, (3, 63, 3, 1536)))
        path.write_bytes(original)
    path.write_bytes(original[:-1])
    reject(lambda: compare.read(native, path.name, compare.NATIVE_MAGIC, (3, 63, 3, 1536)))
    path.write_bytes(original)
    path.rename(native/'extra.bin'); reject(); (native/'extra.bin').rename(path)
    marker = native/'state-complete.bin'
    marker_bytes = marker.read_bytes()
    marker.unlink(); reject(); marker.write_bytes(marker_bytes)
    path.unlink(); path.symlink_to(cpu/path.name); reject(); path.unlink(); path.write_bytes(original)
    first = native/'c2-l0-k0.bin'
    before = first.read_bytes()
    changed = bytearray(before); changed[64] ^= 1
    first.write_bytes(changed); reject(); first.write_bytes(before)
    compare.compare(cpu, native)
    # No family/version auto-detection. A legacy flag cannot promote v2 or
    # a CPU corpus,and default mode refuses every old-v1 marker/record.
    reject(lambda: compare.compare(cpu, legacy))
    reject(lambda: compare.compare(cpu, native, legacy_raw=True))
    reject(lambda: compare.compare(cpu, cpu, legacy_raw=True))
    reject(lambda: compare.read(legacy, 'c2-l0-k0.bin', compare.NATIVE_MAGIC, (2, 0, 0, 786432)))
    legacy_file = legacy/'c2-l0-k0.bin';raw = legacy_file.read_bytes()
    changed = bytearray(raw);changed[64+4*64] ^= 1;legacy_file.write_bytes(changed)
    reject(lambda: compare.compare(cpu, legacy, legacy_raw=True));legacy_file.write_bytes(raw)
    # Reject assertion-disabled invocation before any artifact is read.
    result = subprocess.run([sys.executable, '-O', str(here/'compare_pioneer_q38_state_snapshots.py'), str(cpu), str(native)], capture_output=True, text=True)
    assert result.returncode and 'assertions disabled' in result.stderr
    assert manifest(legacy) == legacy_before
    print(f'PASS strict DATA comparator/{rejected} changed-bit,identity,nonfinite,length,missing,extra,marker,symlink refusals;SYNTHETIC ONLY,no original-model or hardware qualification')
    print('PASS nativev2logical DEFAULT/old nativev1raw EXPLICIT ONLY/CPUv1 unchanged/read-only bit permutation/prohibited version mixing/optimized-Python refusal;NO legacy file rewrite or producer-origin/execution authority')
