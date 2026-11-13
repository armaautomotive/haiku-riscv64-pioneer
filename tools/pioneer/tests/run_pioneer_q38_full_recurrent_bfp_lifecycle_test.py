#!/usr/bin/env python3
"""Reproduce the pinned offline recurrent-attention simulation; no hardware."""
import ast
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert __debug__ and (len(sys.argv) == 12 or (len(sys.argv) == 13 and sys.argv[12] in ('--controller-input','--capture-controller','--capture-controller-reuse','--capture-controller-reuse-prefixed')))
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, worker, leaf, private, gate, qkv, alpha, beta, learned, output, conv = map(Path, sys.argv[1:12])
mode = sys.argv[12] if len(sys.argv) == 13 else None
external = sys.stdin.buffer.read() if mode else None
if external is not None:
    expected = 96 * 4 if mode == '--controller-input' else 2 * 42 * 64 * 4
    if mode == '--capture-controller-reuse-prefixed': expected += 2 * (20 * 512 + 119 * 256)
    assert len(external) == expected
here = Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and not sdk.is_symlink()
pins = (
    (worker / 'program.elf', '67a9bf22e0f9713b699bb058db3cc5702f4940fb786845e66a3e89a3bfe2175d'),
    (leaf / 'program.elf', 'e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
    (private / 'PioneerNativeBfpModelBandData.h', '396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a'),
    (private / 'PioneerNativeBfpModelBandMetadata.h', 'a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d'),
    (gate, 'e3c1189211445020054aeb42ccd89e0d968fb39acd35d0be99ff64755fb1f02b'),
    (qkv, '6146121bb87b7a684d544b73e06a62f0a6b05513866715bfdbff3dfc840d514a'),
    (alpha, '92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb'),
    (beta, 'f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753'),
    (learned, 'e850e588b4ea985d75895f4f188d22ef777a095fd5ca49b9d3a797702a46a311'),
    (output, '8158c4fbaa3569ae7f3453d367b7d988a6d67d0107817c7af524308f1814d886'),
    (conv / 'program.elf', '6cb628a5edf0f40199b4c6528c3ef422da26462c325159fca84dafabca993b2c'),
)

def verify():
    for path, digest in pins:
        assert path.is_absolute() and path.is_file() and not path.is_symlink(), path
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, path
    for path, _ in pins[2:10]:
        info = path.lstat()
        assert info.st_uid == os.geteuid() and info.st_nlink == 1 and stat.S_IMODE(info.st_mode) == 0o600, path

verify()
tree = ast.parse((here / 'run_pioneer_q38_recurrent_small_dispatch_test.py').read_text())
roles = next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign)
             and any(isinstance(target, ast.Name) and target.id == 'roles' for target in node.targets))
with tempfile.TemporaryDirectory(prefix='q38-full-recurrent-', dir='/private/tmp') as tmp:
    binary = Path(tmp) / 'test'
    tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    assert subprocess.check_output([tool + 'nm', '-n', str(worker / 'program.elf')]) == (worker / 'program.symbols').read_bytes()
    for image, sections in ((worker, ('boot', 'globalworker', 'bf16producer', 'cachedfpu', 'dense', 'text', 'affine', 'ffn', 'service', 'gdn')),
                            (leaf, ('boot', 'text')), (conv, ('boot', 'text'))):
        for section in sections:
            extracted = Path(tmp) / 'section.bin'
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section,
                            str(image / 'program.elf'), str(extracted)], check=True)
            filename = 'code.bin' if image == conv and section == 'text' else section + '.bin'
            assert extracted.read_bytes() == (image / filename).read_bytes(), (image, section)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-function', '-Wno-unused-variable', '-Wno-misleading-indentation', '-ffp-contract=off',
                    '-fsanitize=address,undefined', *['-D' + role + '=1' for role in roles], '-I' + str(private),
                    str(here / 'pioneer_q38_full_recurrent_bfp_lifecycle_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *map(str, (worker, leaf, gate, qkv, alpha, beta, learned, output, conv)),
                    *([mode] if mode else [])], input=external, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
verify()
