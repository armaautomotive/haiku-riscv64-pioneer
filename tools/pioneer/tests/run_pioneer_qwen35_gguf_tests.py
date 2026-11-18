#!/usr/bin/env python3
"""Test exact 4B packed reader and reject corrupted sparse GGUF headers."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import qwen35_4b_p100a_preflight as model


def unique(data, needle):
    at = data.find(needle)
    if at < 0 or data.find(needle, at + 1) >= 0:
        raise ValueError('fixture marker absent or ambiguous')
    return at


def encoded(text):
    data = text.encode('ascii')
    return struct.pack('<Q', len(data)) + data


def main(path):
    with tempfile.TemporaryDirectory(prefix='qwen35-gguf-') as tmp:
        binary = str(Path(tmp) / 'reader')
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g',
                        '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                        str(HERE / 'pioneer_qwen35_gguf_test.cpp'),
                        str(HERE.parent / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
        subprocess.run([binary, path], check=True)
        with open(path, 'rb') as source:
            header = source.read(model.MAX_HEADER)
        parsed = model.inspect(header, model.MODEL_BYTES, True)
        model.inventory(parsed)
        header = header[:parsed['data_start']]
        name = encoded('blk.0.attn_qkv.weight')
        descriptor = unique(header, name) + len(name)
        embedding = encoded('token_embd.weight')
        embed_desc = unique(header, embedding) + len(embedding)
        meta = encoded('qwen35.block_count')
        block_value = unique(header, meta) + len(meta) + 4
        name_meta = encoded('general.name') + struct.pack('<I', 8) + encoded('Qwen3.5-4B')
        model_name = unique(header, name_meta) + len(name_meta) - len('Qwen3.5-4B')
        mutations = [
            (model_name, b'X'),
            (block_value, struct.pack('<I', 64)),
            (descriptor, struct.pack('<I', 3)),
            (descriptor + 4, struct.pack('<Q', 256)),
            (descriptor + 4 + 16, struct.pack('<I', 12)),
            (embed_desc + 4 + 16 + 4, struct.pack('<Q', 0)),
            (unique(header, name) + 8, b'X'),
        ]
        fixture = Path(tmp) / 'invalid.gguf'
        for at, replacement in mutations:
            bad = bytearray(header)
            bad[at:at + len(replacement)] = replacement
            with fixture.open('wb') as out:
                out.write(bad)
                out.truncate(model.MODEL_BYTES)
            subprocess.run([binary, '--reject', str(fixture)], check=True)
        with fixture.open('wb') as out:
            out.write(header)
            out.truncate(model.MODEL_BYTES)
        subprocess.run([binary, '--mutation', str(fixture)], check=True)
        with fixture.open('wb') as out:
            out.write(header)
        subprocess.run([binary, '--reject', str(fixture)], check=True)
        print('PASS eight malformed headers/sizes rejected and source mutation detected; no device access')


if __name__ == '__main__':
    if not __debug__ or len(sys.argv) != 2:
        raise SystemExit('use PINNED_QWEN35_4B_GGUF with assertions enabled')
    main(sys.argv[1])
