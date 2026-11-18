#!/usr/bin/env python3
"""Off-card packed panel tests using the authenticated GGUF, never inference."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import qwen35_4b_p100a_preflight as model


def main(path):
    with tempfile.TemporaryDirectory(prefix='qwen35-q5-panel-') as tmp:
        binary = str(Path(tmp) / 'test')
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g',
                        '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', str(HERE / 'pioneer_qwen35_q5_panel_test.cpp'),
                        '-o', binary], check=True)
        subprocess.run([binary], check=True)
        existing = str(Path(tmp) / 'existing')
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g',
                        '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', str(HERE / 'pioneer_qwen35_existing_panel_test.cpp'),
                        '-o', existing], check=True)
        fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
        with os.fdopen(fd, 'rb') as source:
            before = os.fstat(fd)
            if not stat.S_ISREG(before.st_mode) or before.st_size != model.MODEL_BYTES:
                raise ValueError('wrong model file')
            digest = hashlib.sha256()
            while block := source.read(8 * 1024 * 1024):
                digest.update(block)
            if digest.hexdigest() != model.MODEL_SHA or model.identity(os.fstat(fd)) != model.identity(before):
                raise ValueError('model authentication or identity failed')
            source.seek(0)
            parsed = model.inspect(source.read(model.MAX_HEADER), before.st_size, True)
            model.inventory(parsed)
            for fmt, kind, elements, size, count in (('Q5_K', 13, 256, 176, 48),
                    ('Q4_K', 12, 256, 144, 131), ('Q6_K', 14, 256, 210, 22),
                    ('Q8_0', 8, 32, 34, 48)):
                tensors = [t for t in parsed['tensors'] if t['format'] == fmt]
                if len(tensors) != count:
                    raise ValueError('unexpected tensor count')
                samples = bytearray()
                for tensor in tensors:
                    width, rows = tensor['shape']
                    blocks = width // elements
                    for first in (0, (rows // 32) * 16, rows - 16):
                        for index in (0, blocks // 2, blocks - 1):
                            for row in range(first, first + 16):
                                source.seek(tensor['file_offset'] + (row * blocks + index) * size)
                                part = source.read(size)
                                if len(part) != size:
                                    raise ValueError('short quantized block')
                                samples.extend(part)
                if model.identity(os.fstat(fd)) != model.identity(before):
                    raise ValueError('model changed while sampling')
                command = [binary, '--stdin'] if kind == 13 else [existing, str(kind)]
                subprocess.run(command, input=samples, check=True)
            if model.identity(os.fstat(fd)) != model.identity(before):
                raise ValueError('model changed during test')
    print('PASS same-open full SHA-256 and descriptor validation; all quantized tensors sampled; no device access')


if __name__ == '__main__':
    if not __debug__ or len(sys.argv) != 2:
        raise SystemExit('use PINNED_QWEN35_4B_GGUF with assertions enabled')
    main(sys.argv[1])
