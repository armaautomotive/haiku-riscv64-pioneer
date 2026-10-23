#!/usr/bin/env python3
"""Build/run actual private producer+projection execution owner offline."""
import os
import hashlib
import pathlib
import re
import runpy
import struct
import subprocess
import sys
import tempfile
here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) not in (6, 7) or (len(sys.argv) == 7 and sys.argv[6] not in ('--faults', '--wide')):
    raise SystemExit('expected QPR1 QFP3 ZERO QPE1 QRN1 [--faults|--wide]')
pins = runpy.run_path(str(here.parent / 'generate_qwen38_projection_graph_images.py'))['PINS']
for name, (section, size, sha, _) in pins.items():
    root = pathlib.Path(sys.argv[1 if name.startswith('root-') else 2])
    data = (root / (section + '.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
    assert (root / 'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
source = (here / 'run_pioneer_q38_projection_graph_cooperative_tests.sh').read_text()
flags = sorted(set(re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', source)))
flags.remove('-DPIONEER_Q38_TEST_PROJECTION_FOOTPRINT')
with tempfile.TemporaryDirectory(prefix='pioneer-projection-execution-test-') as temporary:
    base = pathlib.Path(temporary)
    native = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
    for prefix, names in [('embedding', ('scope', 'unsafe', 'mapping', 'factory', 'end')), ('rms', ('scope', 'factory'))]:
        for name in names:
            marker = 'PIONEER_Q38_' + ('EMBEDDING' if prefix == 'embedding' else 'LEARNED_RMS') + '_NATIVE_' + name.upper()
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert native.count(begin) == native.count(end) == 1
            (base / (prefix + '_factory_' + name + '.inc')).write_text(native.split(begin, 1)[1].split(end, 1)[0])
    (base / 'embedding_factory_retain.inc').write_text(native.split('// PIONEER_Q38_RETAIN_MEMORY_BEGIN', 1)[1].split('// PIONEER_Q38_RETAIN_MEMORY_END', 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1',
                    '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1', '-DPIONEER_Q38_NORMAL_CLOCK=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-I' + str(base),
                    *[str(here / (name + '.cpp')) for name in ('pioneer_q38_projection_execution_test', 'pioneer_q38_projection_execution_producer',
                      'pioneer_q38_projection_execution_machine', 'pioneer_q38_embedding_execution_bridge', 'pioneer_q38_learned_rms_execution_bridge')],
                    '-o', str(binary)], check=True)
    for fault in (range(13) if sys.argv[-1] == '--faults' else [None]):
        suffix = ['--wide'] if sys.argv[-1] == '--wide' else [] if fault is None else [str(fault)]
        subprocess.run([str(binary), *sys.argv[1:6], *suffix], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
