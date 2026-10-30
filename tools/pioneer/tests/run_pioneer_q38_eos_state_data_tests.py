#!/usr/bin/env python3
"""Synthetic two-position state DATA format/corruption tests; NEVER device IO."""
import os
import pathlib
import struct
import subprocess
import tempfile
import compare_pioneer_q38_eos_state as compare

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-eos-state-data-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    cpu, observed, refused = (base/p for p in ('cpu', 'observed', 'refused'))
    for directory in (cpu, observed, refused):
        directory.mkdir(mode=0o700)
    binary = base/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    str(here/'pioneer_q38_eos_state_data_test.cpp'), str(here/'pioneer_q38_eos_state_data_bridge.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(cpu), str(observed), str(refused)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    assert not list(refused.iterdir())
    compare.compare(cpu, observed)
    faults = 0
    def reject(operation):
        global faults
        try:
            operation()
        except (ValueError, OSError):
            faults += 1
        else:
            raise AssertionError('bad EOS state accepted')
    path = observed/'eos-c2-l63-k3.bin'
    original = path.read_bytes()
    for offset, value in ((0, 0x31534e51), (0, compare.CPU_MAGIC), (4, 2), (8, 3), (12, 62),
                          (16, 2), (20, 1023), (24, 1), (64, 0x7c000000), (64, 0x00007c00), (len(original)-4, 0)):
        changed = bytearray(original);struct.pack_into('<I', changed, offset, value);path.write_bytes(changed)
        reject(lambda: compare.read(observed, path.name, compare.OBSERVED_MAGIC, (63, 3, 1024)))
        path.write_bytes(original)
    path.write_bytes(original[:-1]);reject(lambda: compare.read(observed, path.name, compare.OBSERVED_MAGIC, (63, 3, 1024)));path.write_bytes(original)
    path.rename(observed/'extra.bin');reject(lambda: compare.compare(cpu, observed));(observed/'extra.bin').rename(path)
    marker = observed/'eos-state-complete.bin';saved = marker.read_bytes()
    marker.unlink();reject(lambda: compare.compare(cpu, observed));marker.write_bytes(saved)
    marker.write_bytes(struct.pack('<16I', 0x31534e51, 1, 176, 3, 64, *([0]*11))+struct.pack('<I', compare.END))
    reject(lambda: compare.compare(cpu, observed));marker.write_bytes(saved)
    path.unlink();path.symlink_to(cpu/path.name);reject(lambda: compare.read(observed, path.name, compare.OBSERVED_MAGIC, (63, 3, 1024)));path.unlink();path.write_bytes(original)
    first = observed/'eos-c2-l0-k0.bin';before = first.read_bytes();changed = bytearray(before);changed[64] ^= 1;first.write_bytes(changed)
    reject(lambda: compare.compare(cpu, observed));first.write_bytes(before)
    changed = bytearray(before);struct.pack_into('<I', changed, 64, 0x7f800000);first.write_bytes(changed)
    reject(lambda: compare.read(observed, first.name, compare.OBSERVED_MAGIC, (0, 0, 786432)));first.write_bytes(before)
    compare.compare(cpu, observed)
    print(f'PASS EOS state strict DATA parser/{faults} family,identity,nonfinite,length,missing,extra,marker,symlink,changed-bit refusals;SYNTHETIC ONLY,NO original-model or hardware result')
