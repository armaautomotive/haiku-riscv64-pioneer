#!/usr/bin/env python3
"""Compile/run ACTUAL kernel methods with synthetic MMIO/map primitives."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent
driver = directory.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
text = driver.read_text()
begin, end = '// PIONEER_Q38_FFN_CODE_NATIVE_ADAPTER_BEGIN', '// PIONEER_Q38_FFN_CODE_NATIVE_ADAPTER_END'
if text.count(begin) != 1 or text.count(end) != 1:
    raise SystemExit('STOP exact native adapter markers missing/ambiguous')
snippet = text.split(begin, 1)[1].split(end, 1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-native-adapter-') as temporary:
    base = pathlib.Path(temporary)
    include = base / 'actual-native-methods.inc'
    include.write_text(snippet)
    binary = base / 'test'
    command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=address,undefined', '-DPIONEER_FFN_NATIVE_ADAPTER_INCLUDE="' + str(include) + '"',
               str(directory / 'pioneer_q38_ffn_code_native_adapter_test.cpp'), '-o', str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
