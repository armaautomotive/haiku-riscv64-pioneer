#!/usr/bin/env python3
"""Actual native scope/retention/END extraction; NO device access."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) != 4:
    raise SystemExit('exact frozen QFG2/QFP2/zero image directories required')
subprocess.run(['sh', str(here / 'run_pioneer_q38_ffn_input_graph_images_tests.sh'), *sys.argv[1:3]], check=True)
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-graph-native-scope-') as directory:
    base = pathlib.Path(directory)
    for filename, marker in [('native_graph_scope', 'PIONEER_Q38_FFN_GRAPH_NATIVE_SCOPE'),
                             ('native_graph_retain', 'PIONEER_Q38_RETAIN_MEMORY'),
                             ('native_graph_end', 'PIONEER_Q38_FFN_GRAPH_END_GATE')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        if source.count(begin) != 1 or source.count(end) != 1:
            raise SystemExit('STOP actual native section missing/ambiguous: ' + marker)
        (base / (filename + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1', '-DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1',
                    '-DPIONEER_Q38_GDN_MUTABLE_STATE', '-DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION', '-DPIONEER_Q38_TEST_AFFINE_REPLAY',
                    '-DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE', '-DPIONEER_Q38_TEST_INT_ACCUMULATOR', '-DPIONEER_Q38_TEST_PIPELINED_MATRIX',
                    '-I' + str(base), str(here / 'pioneer_q38_ffn_graph_native_scope_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
