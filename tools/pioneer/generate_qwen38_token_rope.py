#!/usr/bin/env python3
"""Freeze raw8192-position RoPE configuration DATA; never device/model IO."""
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
output = pathlib.Path(sys.argv[1])
assert output.is_absolute() and not output.is_symlink()
with tempfile.TemporaryDirectory(prefix='pioneer-token-rope-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'generator'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', str(here/'qwen38_token_rope_constants.cpp'),
                    '-o', str(binary)], check=True)
    raw = subprocess.check_output([str(binary)])
assert len(raw) == 8192*64*4
words = struct.unpack('<524288I', raw)
assert all((w & 0x7fffffff) <= 0x3f800000 for w in words)
assert words[:64] == (0x3f800000, 0)*32
digest = hashlib.sha256(raw).hexdigest()
assert digest == '23abf02bc6bfede090d3434921b2fd9e84b2e9c46e5ae8cb033658740a6305a9', 'libm/compiler raw bits differ from the frozen artifact;REFUSING regeneration'
lines = ['/* SPDX-License-Identifier: MIT */',
         '// GENERATED fixed raw model configuration ONLY;NO learned/hidden/KV products.',
         '//8192 positions x32 frequencies x(cos,sin),IEEE binary32 little-endian.',
         '// SHA256 '+digest,
         '#ifndef PIONEER_Q38_TOKEN_ROPE_H', '#define PIONEER_Q38_TOKEN_ROPE_H',
         '#include <stdint.h>', 'namespace PioneerQ38TokenRope {',
         'enum {Positions=8192,WordsPerPosition=64,Words=524288,Bytes=2097152};',
         'static const uint32_t Data[Words]={']
for at in range(0, len(words), 8):
    lines.append(' '+','.join('0x%08xU' % w for w in words[at:at+8])+',')
lines += ['};', 'static_assert(sizeof(Data)==Bytes,"complete immutable raw RoPE DATA");',
          '}', '#endif', '']
text = '\n'.join(lines)
if output.exists():
    assert output.read_text() == text, 'existing frozen artifact differs;REFUSING replacement'
else:
    output.write_text(text)
print('PASS generated frozen raw RoPE bytes2097152/positions8192/finite bounded constants/sha256'+digest+
      ';NOT normalized/rotated activations/model products/native ownership/execution/TPS')
