#!/usr/bin/env python3
"""Compile the ACTUAL kernel hook+retention with simulated endpoints only."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
source = driver.read_text()

def extract(begin, end):
    if source.count(begin) != 1 or source.count(end) != 1:
        raise SystemExit('STOP exact native markers missing/ambiguous')
    return source.split(begin, 1)[1].split(end, 1)[0]

with tempfile.TemporaryDirectory(prefix='pioneer-ffn-root-native-owner-') as temporary:
    base = pathlib.Path(temporary)
    hook = base / 'native-hook.inc'
    retain = base / 'native-retain.inc'
    end_gate = base / 'native-end-gate.inc'
    hook.write_text(extract('// PIONEER_Q38_FFN_ROOT_LIFETIME_NATIVE_OWNER_BEGIN', '// PIONEER_Q38_FFN_ROOT_LIFETIME_NATIVE_OWNER_END'))
    retain.write_text(extract('// PIONEER_Q38_RETAIN_MEMORY_BEGIN', '// PIONEER_Q38_RETAIN_MEMORY_END'))
    end_gate.write_text(extract('// PIONEER_Q38_FFN_ROOT_LIFETIME_END_GATE_BEGIN', '// PIONEER_Q38_FFN_ROOT_LIFETIME_END_GATE_END'))
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-DPIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC=1', '-DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1',
                    '-DPIONEER_ROOT_NATIVE_OWNER_INCLUDE="' + str(hook) + '"',
                    '-DPIONEER_ROOT_RETAIN_INCLUDE="' + str(retain) + '"',
                    '-DPIONEER_ROOT_END_GATE_INCLUDE="' + str(end_gate) + '"',
                    str(here / 'pioneer_q38_ffn_root_lifetime_native_owner_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
