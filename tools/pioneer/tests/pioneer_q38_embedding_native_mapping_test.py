#!/usr/bin/env python3
"""Extract actual native root mapping methods;simulate ONLY primitives."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
begin, end = '// PIONEER_Q38_EMBEDDING_NATIVE_MAPPING_BEGIN', '// PIONEER_Q38_EMBEDDING_NATIVE_MAPPING_END'
assert source.count(begin) == source.count(end) == 1
with tempfile.TemporaryDirectory(prefix='pioneer-embedding-native-map-') as temporary:
    base = pathlib.Path(temporary)
    (base / 'embedding_native_mapping.inc').write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I' + str(base), str(here / 'pioneer_q38_embedding_native_mapping_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
