#!/usr/bin/env python3
"""Pinned actual packet/full guards,synthetic ABI ONLY;no model/device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
baseline, candidate = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() and not p.is_symlink() for p in (baseline, candidate))
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'


def pins(path):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
                if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))


maps = []
for directory, script in ((baseline, 'build_qwen38_cohort_local_sources_root.py'),
                          (candidate, 'build_qwen38_packet_identity_boundary_root.py')):
    expected = pins(here.parent / script)
    assert expected
    for name, size, digest in expected:
        data = (directory / name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
    symbols = subprocess.check_output([tool + 'nm', '-n', str(directory / 'program.elf')], text=True)
    assert symbols == (directory / 'program.symbols').read_text()
    symbols = [(int(p[0], 16), p[2]) for line in symbols.splitlines() if len(p := line.split()) == 3]

    def one(name):
        found = [a for a, n in symbols if n == name]
        assert len(found) == 1, (name, found)
        return found[0]

    full, packet = one('q38_layer_return_recurrent_seal'), one('q38_layer_return_recurrent_packet_seal')
    fault = one('q38_token_fault_report')
    text = (directory / 'text.bin').read_bytes()
    # Authenticate all actual report instructions,including two permitted
    # status stores and the final inert loop. Never intercept this body.
    words = tuple(int.from_bytes(text[fault - 0x40000 + i:fault - 0x40000 + i + 4], 'little') for i in range(0, 32, 4))
    assert words == (0x000752b7, 0x4612a023, 0x51465337, 0x43130313,
                     0x0ff0000f, 0x4462a423, 0x0ff0000f, 0x0000006f), words
    maps.append((full, packet, full, packet, fault + 28))
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
assert flags
with tempfile.TemporaryDirectory(prefix='pioneer-q38-packet-identity-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    mappings = []
    for i, values in enumerate(maps):
        path = out / f'{i}.map'
        path.write_text(' '.join(format(a, 'x') for a in values) + '\n')
        mappings.append(path)
    binary = out / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags,
                    str(here / 'pioneer_q38_packet_identity_boundary_instruction_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(baseline), str(candidate), *map(str, mappings)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
