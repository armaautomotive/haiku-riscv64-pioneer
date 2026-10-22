#!/usr/bin/env python3
"""Actual native lifecycle extraction and frozen zero RV32; NO device."""
import os
import hashlib
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
sections = {
    'native_lock': 'PIONEER_PROBE_LOCK',
    'native_retain': 'PIONEER_Q38_RETAIN_MEMORY',
    'native_scope': 'PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE',
    'call_scope': 'PIONEER_Q38_GDN_CALL_SCOPE',
    'model_bind': 'PIONEER_Q38_FFN_MODEL_NATIVE_BIND',
    'model_initialize': 'PIONEER_Q38_FFN_MODEL_NATIVE_INITIALIZE',
    'model_control': 'PIONEER_Q38_FFN_MODEL_NATIVE_CONTROL',
    'model_end': 'PIONEER_Q38_FFN_MODEL_END_GATE',
}
if len(sys.argv) != 2:
    raise SystemExit('one exact frozen zero-image directory required')
for name, digest in [('boot.bin', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
                     ('text.bin', '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b')]:
    if hashlib.sha256((pathlib.Path(sys.argv[1]) / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('STOP frozen zero image hash mismatch: ' + name)
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-model-native-') as directory:
    base = pathlib.Path(directory)
    for name, marker in sections.items():
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        if source.count(begin) != 1 or source.count(end) != 1:
            raise SystemExit('STOP missing/ambiguous actual native section: ' + marker)
        (base / (name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_GDN_MUTABLE_STATE', '-DPIONEER_Q38_FFN_MODEL_INIT=1',
                    '-DPIONEER_Q38_NORMAL_CLOCK', '-DTT_PIONEER_DRIVER_RELOAD', '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS',
                    '-I' + str(base), str(here / 'pioneer_q38_ffn_model_native_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), sys.argv[1]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
