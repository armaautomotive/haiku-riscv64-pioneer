#!/usr/bin/env python3
"""Actual native scope/Unsafe/END bodies with real controller/ROM machines."""
import os
import hashlib
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) != 3:
    raise SystemExit('expected ZERO and QPE1 image directories')
assert hashlib.sha256((pathlib.Path(sys.argv[1]) / 'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
assert hashlib.sha256((pathlib.Path(sys.argv[2]) / 'code.bin').read_bytes()).hexdigest() == 'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-embedding-native-scope-') as temporary:
    base = pathlib.Path(temporary)
    for name, marker in [('scope', 'SCOPE'), ('unsafe', 'UNSAFE'), ('end', 'END')]:
        begin, end = '// PIONEER_Q38_EMBEDDING_NATIVE_' + marker + '_BEGIN', '// PIONEER_Q38_EMBEDDING_NATIVE_' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / ('embedding_native_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-I' + str(base),
                    str(here / 'pioneer_q38_embedding_native_scope_test.cpp'), str(here / 'pioneer_q38_embedding_execution_bridge.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
