#!/usr/bin/env python3
"""Execute ACTUAL native factory path with real state and emitted RISC VMs."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] not in ('--faults', '--control', '--validation')):
    raise SystemExit('expected ZERO QPE1 [--faults|--control|--validation]')
assert hashlib.sha256((pathlib.Path(sys.argv[1]) / 'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
assert hashlib.sha256((pathlib.Path(sys.argv[2]) / 'code.bin').read_bytes()).hexdigest() == 'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-embedding-native-factory-') as temporary:
    base = pathlib.Path(temporary)
    for name, marker in [('scope', 'PIONEER_Q38_EMBEDDING_NATIVE_SCOPE'), ('unsafe', 'PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE'),
                         ('mapping', 'PIONEER_Q38_EMBEDDING_NATIVE_MAPPING'), ('factory', 'PIONEER_Q38_EMBEDDING_NATIVE_FACTORY'),
                         ('end', 'PIONEER_Q38_EMBEDDING_NATIVE_END'), ('retain', 'PIONEER_Q38_RETAIN_MEMORY'),
                         ('probe_lock', 'PIONEER_PROBE_LOCK'), ('call_scope', 'PIONEER_Q38_GDN_CALL_SCOPE'), ('control', 'PIONEER_Q38_EMBEDDING_NATIVE_CONTROL'),
                         ('routing', 'PIONEER_Q38_EMBEDDING_NATIVE_ROUTING')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / ('embedding_factory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    validation = sys.argv[-1] == '--validation'
    fixture = 'validation' if validation else 'control' if sys.argv[-1] == '--control' else 'factory'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-DPIONEER_Q38_EMBEDDING_NATIVE=1',
                    '-DPIONEER_Q38_NORMAL_CLOCK=1', '-I' + str(base),
                    *(['-DPIONEER_Q38_EMBEDDING_VALIDATION_NATIVE=1'] if validation else []),
                    str(here / ('pioneer_q38_embedding_native_' + fixture + '_test.cpp')),
                    str(here / 'pioneer_q38_embedding_execution_bridge.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:3], *([] if sys.argv[-1] in ('--control', '--validation') else sys.argv[3:])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
