#!/usr/bin/env python3
"""Strict posthoc DATA comparison; never a native/hardware execution proof."""
import array
import argparse
import hashlib
import os
import pathlib
import stat
import struct
import sys

CPU_MAGIC, NATIVE_MAGIC, END = 0x31534351, 0x31534e51, 0x444e4553
NATIVE_VERSION = 2
LEGACY_COLLECTOR_SHA = '21dd8c450a046959bb773751e22925643c8b8f87d29d75ce53f5ad8e73968d6b'
if not __debug__:
    raise RuntimeError('State DATA verifier must not run with assertions disabled')


def corpus():
    expected = {}
    for completed in (2, 3):
        for layer in range(64):
            kinds = (0,) if completed == 2 and layer % 4 != 3 else () if completed == 2 else (2, 3) if layer % 4 == 3 else (0, 1)
            for kind in kinds:
                count = 786432 if kind == 0 else 30720 if kind == 1 else completed * 512
                expected[f'c{completed}-l{layer}-k{kind}.bin'] = (completed, layer, kind, count)
    assert len(expected) == 176
    return expected


def stable(path, size):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    with os.fdopen(fd, 'rb') as source:
        before = os.fstat(source.fileno())
        assert stat.S_ISREG(before.st_mode) and before.st_size == size, f'nonregular/wrong-size state DATA: {path}'
        data = source.read(size+1)
        after = os.fstat(source.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        assert identity(before) == identity(after) and len(data) == size, f'state DATA changed while reading: {path}'
        return data


def read(directory, name, magic, fields, version=None):
    path = directory / name
    completed, layer, kind, count = fields
    if version is None:
        version = NATIVE_VERSION if magic == NATIVE_MAGIC else 1
    assert (magic == CPU_MAGIC and version == 1) or (magic == NATIVE_MAGIC and version in (1, NATIVE_VERSION)), 'unsupported state DATA family/version'
    data = stable(path, (16 + count + 1) * 4)
    assert struct.unpack_from('<16I', data) == (magic, version, completed, layer, kind, count, *([0] * 10)), f'header/identity mismatch: {path}'
    assert struct.unpack_from('<I', data, len(data) - 4)[0] == END, f'footer mismatch: {path}'
    words = array.array('I')
    assert words.itemsize == 4, 'state DATA requires uint32 arrays'
    words.frombytes(data[64:-4])
    if sys.byteorder != 'little':
        words.byteswap()
    if kind < 2:
        assert all(word & 0x7f800000 != 0x7f800000 for word in words), f'nonfinite FP32: {path}'
    else:
        assert all(word & 0x7c00 != 0x7c00 and word & 0x7c000000 != 0x7c000000 for word in words), f'nonfinite packed binary16: {path}'
    return data[64:-4], hashlib.sha256(data).hexdigest()


def legacy_gdn_to_logical(data):
    # Read-only,bit-preserving permutation of the documented old collector's
    # [head][half][key][value64] bytes,NOT numerical repair or model execution.
    assert len(data) == 786432*4, 'legacy raw GDN shape'
    raw = array.array('I');assert raw.itemsize == 4;raw.frombytes(data)
    if sys.byteorder != 'little':
        raw.byteswap()
    logical = array.array('I', [0])*786432
    for head in range(48):
        for key in range(128):
            target = head*16384+key*128
            source = head*16384+key*64
            logical[target:target+64] = raw[source:source+64]
            logical[target+64:target+128] = raw[source+8192:source+8192+64]
    if sys.byteorder != 'little':
        logical.byteswap()
    return logical.tobytes()


def compare(cpu, native, legacy_raw=False):
    expected = corpus()
    native_version = 1 if legacy_raw else NATIVE_VERSION
    marker = lambda magic, version: struct.pack('<16I', magic, version, 176, 3, 64, *([0] * 11)) + struct.pack('<I', END)
    for directory, magic, version in ((cpu, CPU_MAGIC, 1), (native, NATIVE_MAGIC, native_version)):
        assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), f'nonregular directory: {directory}'
        assert {p.name for p in directory.iterdir()} == set(expected) | {'state-complete.bin'}, f'missing/extra state records: {directory}'
        data = marker(magic, version)
        assert stable(directory/'state-complete.bin', len(data)) == data, f'incomplete/invalid versioned marker: {directory}'
    total = 0
    for name, fields in expected.items():
        original, _ = read(cpu, name, CPU_MAGIC, fields)
        observed, _ = read(native, name, NATIVE_MAGIC, fields, native_version)
        if legacy_raw and fields[2] == 0:
            observed = legacy_gdn_to_logical(observed)
        assert observed == original, f'BIT MISMATCH {name};NO tolerance,repair or diagnostic-only pass'
        total += fields[3]
    print(f'PASS176 state DATA records/{total} words BIT-EXACT/nativeVersion{native_version}/legacyRawGdnReadOnly{int(legacy_raw)}/GDN after2+3,Conv after3,packed half K+V all3positions;separate source/execution/STOP/END evidence REQUIRED;NOT hardware,Work or TPS qualification')
    if legacy_raw:
        print(f'LEGACY_LAYOUT_ONLY oldCollectorSourceSHA{LEGACY_COLLECTOR_SHA};source label/layout interpretation is NOT actual producer/execution/origin proof;NO artifact rewritten')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy-raw-gdn', action='store_true', help='Explicitly read old native-v1 half-head DATA;never rewrite files or imply execution provenance')
    parser.add_argument('cpu', type=pathlib.Path);parser.add_argument('native', type=pathlib.Path)
    args = parser.parse_args()
    compare(args.cpu, args.native, legacy_raw=args.legacy_raw_gdn)
