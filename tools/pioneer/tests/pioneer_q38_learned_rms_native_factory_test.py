#!/usr/bin/env python3
"""ACTUAL private native factory+RISC instructions;NO device access."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
actual = len(sys.argv) == 6 and sys.argv[4] == '--actual-model'
if not actual and (len(sys.argv) not in (4, 5) or (len(sys.argv) == 5 and sys.argv[4] not in ('--faults', '--validation', '--control'))):
    raise SystemExit('expected ZERO QPE1 QRN1 [--faults|--validation|--control|--actual-model GGUF]')
validation = actual or sys.argv[-1] in ('--validation', '--control')
for directory, name, expected in zip(sys.argv[1:4], ('text.bin', 'code.bin', 'code.bin'), (
        '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b',
        'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1',
        'f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a')):
    assert hashlib.sha256((pathlib.Path(directory) / name).read_bytes()).hexdigest() == expected
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-rms-native-factory-') as temporary:
    base = pathlib.Path(temporary)
    for prefix, names in [('embedding', ('scope', 'unsafe', 'mapping', 'factory', 'end')), ('rms', ('scope', 'factory'))]:
        for name in names:
            marker = 'PIONEER_Q38_' + ('EMBEDDING' if prefix == 'embedding' else 'LEARNED_RMS') + '_NATIVE_' + name.upper()
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            (base / (prefix + '_factory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    for name, marker in [('retain', 'PIONEER_Q38_RETAIN_MEMORY'), ('probe_lock', 'PIONEER_PROBE_LOCK'),
                         ('call_scope', 'PIONEER_Q38_GDN_CALL_SCOPE'), ('control', 'PIONEER_Q38_EMBEDDING_NATIVE_CONTROL'),
                         ('routing', 'PIONEER_Q38_EMBEDDING_NATIVE_ROUTING')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / ('embedding_factory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    oracle = []
    if actual:
        oracle = [str(base / 'original-reference.o')]
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here / 'pioneer_q38_learned_rms_actual_source.cpp'), '-o', oracle[0]], check=True)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-DPIONEER_Q38_EMBEDDING_NATIVE=1',
                    '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_NORMAL_CLOCK=1', '-I' + str(base),
                    *(['-DPIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE=1'] if validation else []),
                    str(here / ('pioneer_q38_learned_rms_actual_model_test.cpp' if actual else
                                'pioneer_q38_learned_rms_native_control_test.cpp' if sys.argv[-1] == '--control' else
                                'pioneer_q38_learned_rms_native_validation_test.cpp' if validation else 'pioneer_q38_learned_rms_native_factory_test.cpp')),
                    str(here / 'pioneer_q38_embedding_execution_bridge.cpp'),
                    str(here / 'pioneer_q38_learned_rms_execution_bridge.cpp'), *oracle, '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:4], *([sys.argv[5]] if actual else [] if validation else sys.argv[4:])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
