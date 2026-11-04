#!/usr/bin/env python3
"""PRIVATE, independent Q6/Q8 source/layout/Fraction check; no device authority."""
import contextlib
import hashlib
import io
import os
import pathlib
import re
import runpy
import stat
import struct
import sys

assert __debug__ and len(sys.argv) == 3
private, original_path = map(pathlib.Path, sys.argv[1:])
assert private.is_absolute() and original_path.is_absolute() and not private.is_symlink()
assert private.stat().st_uid == os.geteuid() and stat.S_IMODE(private.stat().st_mode) == 0o700


def identity(info):
    return (info.st_dev, info.st_ino, info.st_size, info.st_uid, info.st_gid,
            info.st_mode, info.st_nlink, info.st_mtime_ns, info.st_ctime_ns)


def read(name):
    fd = os.open(private / name, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    try:
        before = os.fstat(fd)
        assert stat.S_ISREG(before.st_mode) and before.st_nlink == 1 and before.st_uid == os.geteuid()
        assert stat.S_IMODE(before.st_mode) == 0o600 and before.st_size < 16 * 1024 * 1024
        value = os.pread(fd, before.st_size + 1, 0)
        assert len(value) == before.st_size and identity(before) == identity(os.fstat(fd))
        return value
    finally:
        os.close(fd)


def array(text, name, count):
    match = re.search(r'const uint32_t ' + name + r'\[' + str(count) + r'\]=\{([^}]+)\};', text)
    assert match, name
    words = re.findall(r'0x([0-9a-f]{8})', match[1])
    assert len(words) == count
    return b''.join(struct.pack('<I', int(word, 16)) for word in words)


def table(text, name, expected):
    match = re.search(r'unsigned ' + name + r'\(unsigned s\)\{static const unsigned t\[17\]=\{([^}]+)\};', text)
    assert match and list(map(int, match[1].split(','))) == expected, name


with contextlib.redirect_stdout(io.StringIO()):
    rational = runpy.run_path(str(pathlib.Path(__file__).parent / 'pioneer_native_bfp_arithmetic_test.py'))


def half(bits):
    exponent, mantissa = (bits >> 10) & 31, bits & 1023
    assert exponent != 31
    return (-1 if bits & 0x8000 else 1) * (1024 + mantissa if exponent else mantissa) * rational['power_of_two'](exponent - 25 if exponent else -24)


def fp32(value, negative_zero=False):
    return rational['encode_normal'](value) if value else (0x80000000 if negative_zero else 0)


fd = os.open(original_path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
try:
    source = os.fstat(fd)
    assert stat.S_ISREG(source.st_mode) and source.st_uid == os.geteuid() and source.st_nlink == 1
    assert not source.st_mode & 0o022 and source.st_size == 18973870528
    digest = hashlib.sha256()
    for offset in range(0, source.st_size, 8 * 1024 * 1024):
        part = os.pread(fd, min(8 * 1024 * 1024, source.st_size - offset), offset)
        assert len(part) == min(8 * 1024 * 1024, source.st_size - offset)
        digest.update(part)
    assert digest.hexdigest() == 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'
    assert identity(source) == identity(os.fstat(fd))
    position = 0

    def take(n):
        global position
        assert 0 <= n <= 16 * 1024 * 1024 and position + n <= source.st_size
        value = os.pread(fd, n, position)
        assert len(value) == n
        position += n
        return value

    def number(fmt):
        return struct.unpack('<' + fmt, take(struct.calcsize('<' + fmt)))[0]

    def string():
        count = number('Q')
        assert count <= 1024 * 1024
        return take(count).decode('utf8')

    def metadata(kind, depth=0):
        assert depth < 16
        if kind == 8:
            return string()
        if kind == 9:
            element, count = number('I'), number('Q')
            assert count < 2000000
            for _ in range(count):
                metadata(element, depth + 1)
            return None
        assert kind in (0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12)
        return number({0: 'B', 1: 'b', 2: 'H', 3: 'h', 4: 'I', 5: 'i', 6: 'f', 7: 'B', 10: 'Q', 11: 'q', 12: 'd'}[kind])

    assert take(4) == b'GGUF' and number('I') == 3
    tensor_count, metadata_count = number('Q'), number('Q')
    assert tensor_count == 851 and 0 < metadata_count < 10000
    alignment, keys = 32, set()
    for _ in range(metadata_count):
        key, kind = string(), number('I')
        assert key not in keys
        keys.add(key)
        value = metadata(kind)
        if key == 'general.alignment':
            assert kind == 4
            alignment = value
    assert 0 < alignment <= 4096 and not alignment & (alignment - 1)
    tensors = []
    for _ in range(tensor_count):
        name, rank = string(), number('I')
        assert 1 <= rank <= 4
        dimensions = [number('Q') for _ in range(rank)]
        kind, offset = number('I'), number('Q')
        tensors.append((name, dimensions, kind, offset))
    start = (position + alignment - 1) & -alignment

    widths = [6144] * 3 + [5120] * 6 + [6144] * 3 + [5120] * 5
    types = [14] * 6 + [8] * 11
    rows = [32] * 16 + [16]
    ordinals = [48] * 3 + [0] * 3 + [5] * 3 + [16] * 3 + [49] * 3 + [11] * 2
    firsts = [0, 2560, 5088, 0, 124160, 248288, 0, 5120, 10208, 0, 2560, 5088, 0, 6144, 12256, 0, 32]
    names = ['blk.3.attn_output.weight'] * 3 + ['output.weight'] * 3 + ['blk.0.attn_qkv.weight'] * 3 + ['blk.0.ssm_out.weight'] * 3 + ['blk.3.attn_q.weight'] * 3 + ['blk.0.ssm_alpha.weight'] * 2
    outputs = [5120] * 3 + [248320] * 3 + [10240] * 3 + [5120] * 3 + [12288] * 3 + [48] * 2
    prepared_sizes = [8192] * 6 + [9216] * 11
    data = read('PioneerNativeMixedFpuStreamData.h').decode('ascii')
    meta = read('PioneerNativeMixedFpuStreamMetadata.h').decode('ascii')
    assert 'Version=1,Rows=32,Samples=17,Role=87,Formats=2' in data
    assert 'Rows=32,Samples=17,Role=87,Formats=2' in meta
    for text in (data, meta):
        for name, values in (('Width', widths), ('Type', types), ('RealRows', rows), ('Tensor', ordinals), ('First', firsts)):
            table(text, name, values)
    table(data, 'WeightWords', [k // 256 * n // 4 for k, n in zip(widths, prepared_sizes)])
    table(data, 'Words', [k // 256 * (n + 512) // 4 for k, n in zip(widths, prepared_sizes)])
    phases = logical_weights = real_weights = goldens = 0
    for sample in range(17):
        width, kind, real_rows, ordinal, first, prepared_bytes = widths[sample], types[sample], rows[sample], ordinals[sample], firsts[sample], prepared_sizes[sample]
        name, dims, tensor_kind, offset = tensors[ordinal]
        assert (name, dims, tensor_kind) == (names[sample], [width, outputs[sample]], kind)
        assert first + real_rows <= dims[1]
        blocks, row_bytes = width // 256, 210 if kind == 14 else 272
        raw, packed, vector, expected = [read(f'sample{sample}.{name}.bin') for name in ('original', 'prepared', 'input', 'expected')]
        assert list(map(len, (raw, packed, vector, expected))) == [blocks * real_rows * row_bytes, blocks * prepared_bytes, blocks * 512, 128]
        assert array(data, f'p{sample}', blocks * prepared_bytes // 4) == packed
        assert array(data, f'i{sample}', blocks * 128) == vector and array(meta, f'g{sample}', 32) == expected
        totals = [0] * 32
        for block in range(blocks):
            original = raw[block * real_rows * row_bytes:(block + 1) * real_rows * row_bytes]
            inputs = vector[block * 512:(block + 1) * 512]
            rebuilt = bytearray(prepared_bytes)
            for row in range(32):
                q = original[row * row_bytes:(row + 1) * row_bytes]
                a, b = [], []
                if row < real_rows:
                    source_offset = start + offset + ((first + row) * blocks + block) * row_bytes
                    assert source_offset + row_bytes <= source.st_size and os.pread(fd, row_bytes, source_offset) == q
                    if kind == 14:
                        h = struct.unpack_from('<H', q, 208)[0]
                        for column in range(128):
                            rebuilt[row // 16 * 2048 + column * 16 + row % 16] = q[column]
                        for column in range(64):
                            rebuilt[4096 + row // 16 * 1024 + column * 16 + row % 16] = q[128 + column]
                        for group in range(16):
                            scale = q[192 + group] - (256 if q[192 + group] >= 128 else 0)
                            at = 6144 + (group * 2 + row // 16) * 64 + row % 16 * 4
                            struct.pack_into('<I', rebuilt, at, fp32(half(h) * scale, bool(h & 0x8000) ^ (scale < 0)))
                    else:
                        for group in range(8):
                            h = struct.unpack_from('<H', q, group * 34)[0]
                            for column in range(32):
                                rebuilt[row // 16 * 4096 + (group * 32 + column) * 16 + row % 16] = q[group * 34 + 2 + column]
                            at = 8192 + (group * 2 + row // 16) * 64 + row % 16 * 4
                            struct.pack_into('<I', rebuilt, at, fp32(half(h), bool(h & 0x8000)))
                for column in range(256):
                    if row >= real_rows:
                        bits = 0
                    elif kind == 14:
                        high, group, lane = column // 128, column % 128 // 32, column % 32
                        low = (q[high * 64 + (group & 1) * 32 + lane] >> (group // 2 * 4)) & 15
                        upper = (q[128 + high * 32 + lane] >> (group * 2)) & 3
                        quant = (low | (upper << 4)) - 32
                        scale = q[192 + column // 16] - (256 if q[192 + column // 16] >= 128 else 0)
                        h = struct.unpack_from('<H', q, 208)[0]
                        bits = fp32(half(h) * scale * quant, bool(h & 0x8000) ^ (scale < 0) ^ (quant < 0))
                    else:
                        h = struct.unpack_from('<H', q, column // 32 * 34)[0]
                        quant = q[column // 32 * 34 + 2 + column % 32]
                        quant -= 256 if quant >= 128 else 0
                        bits = fp32(half(h) * quant, bool(h & 0x8000) ^ (quant < 0))
                    high16, tail = divmod(bits, 65536)
                    high16 += int(tail > 32768 or (tail == 32768 and high16 & 1))
                    a.append(high16)
                    input_bits = 0x3e80 + (column * 19 + block * 11) % 384 + (0x8000 if column & 3 == 2 else 0)
                    assert struct.unpack_from('<H', inputs, column * 2)[0] == input_bits
                    b.append(input_bits)
                total = totals[row]
                for first_column in range(0, 256, 16):
                    for phase in range(4):
                        total = rational['instruction'](a[first_column:first_column + 16], b[first_column:first_column + 16], phase, total)
                        phases += 1
                totals[row] = total
            assert packed[block * prepared_bytes:(block + 1) * prepared_bytes] == rebuilt
            logical_weights += 8192
            real_weights += real_rows * 256
            assert identity(source) == identity(os.fstat(fd))
        assert list(struct.unpack('<32I', expected)) == totals, sample
        goldens += 32
        print(f'PASS PRIVATE mixed export sample{sample} type{kind} tensor{ordinal} first{first} rows{real_rows} K{width}:same-open original/exact recipe/32 Fraction goldens;sha' + hashlib.sha256(packed + vector).hexdigest(), flush=True)
    assert (real_weights, logical_weights, phases, goldens) == (2899968, 2981888, 745472, 544)
    assert identity(source) == identity(os.fstat(fd))
    print('PASS PRIVATE independent mixed87 export:17 original-bound bands/2899968 real weights/2981888 including absent-row padding/745472 rounded phases/544 goldens for1088 two-job words;full original SHA and nanosecond FD identity;NO shared CXX codec/preparer/FPU oracle/weight bytes printed;NOT native/hardware/full-model-quality/speed authority')
finally:
    os.close(fd)
