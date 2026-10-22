#!/usr/bin/env python3
"""Actual native projection scope/retention/END bodies + real zero fixture."""
import os
import hashlib
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen FFN zero image directory')
assert hashlib.sha256((pathlib.Path(sys.argv[1]) / 'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-projection-memory-scope-') as temporary:
    base = pathlib.Path(temporary)
    for name, marker in [('scope', 'PIONEER_Q38_PROJECTION_MEMORY_NATIVE_SCOPE'), ('unsafe', 'PIONEER_Q38_PROJECTION_MEMORY_UNSAFE'),
                         ('retain', 'PIONEER_Q38_RETAIN_MEMORY'), ('end', 'PIONEER_Q38_PROJECTION_MEMORY_END_GATE')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / ('projection_memory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1', '-DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1',
                    '-DPIONEER_Q38_GDN_MUTABLE_STATE', '-DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION', '-DPIONEER_Q38_TEST_AFFINE_REPLAY',
                    '-DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE', '-DPIONEER_Q38_TEST_INT_ACCUMULATOR', '-DPIONEER_Q38_TEST_PIPELINED_MATRIX',
                    '-I' + str(base), str(here / 'pioneer_q38_projection_memory_native_scope_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), sys.argv[1]], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
