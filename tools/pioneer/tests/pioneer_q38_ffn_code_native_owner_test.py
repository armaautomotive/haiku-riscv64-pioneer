#!/usr/bin/env python3
"""Actual kernel call-site extraction;simulated owned context,NO card IO."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent
driver = directory.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
text = driver.read_text()
begin, end = '// PIONEER_Q38_FFN_CODE_NATIVE_OWNER_BEGIN', '// PIONEER_Q38_FFN_CODE_NATIVE_OWNER_END'
if text.count(begin) != 1 or text.count(end) != 1:
    raise SystemExit('STOP exact native owner markers missing/ambiguous')
snippet = text.split(begin, 1)[1].split(end, 1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-native-owner-') as temporary:
    base = pathlib.Path(temporary)
    include = base / 'actual-native-owner.inc'
    include.write_text(snippet)
    for role, flags in [('basic', []), ('qualified', ['-DPIONEER_Q38_TEST_QUALIFIED_LEGACY'])]:
        binary = base / role
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', *flags,
                    '-fsanitize=address,undefined', '-DPIONEER_FFN_NATIVE_OWNER_INCLUDE="' + str(include) + '"',
                    str(directory / 'pioneer_q38_ffn_code_native_owner_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
