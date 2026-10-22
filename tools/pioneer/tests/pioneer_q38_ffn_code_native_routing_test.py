#!/usr/bin/env python3
"""Actual kernel routing and protocol isolation;NO SDK/device access."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent
root = directory.parents[2]
text = (root / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
begin, end = '// PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_BEGIN', '// PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_END'
if text.count(begin) != 1 or text.count(end) != 1:
    raise SystemExit('STOP exact native routing markers missing/ambiguous')
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-native-routing-') as temporary:
    base = pathlib.Path(temporary)
    include = base / 'actual-native-routing.inc'
    include.write_text(text.split(begin, 1)[1].split(end, 1)[0])
    cxx = os.environ.get('CXX', 'c++')
    for mode, flags in [('ordinary', []), ('code', ['-DPIONEER_Q38_FFN_CODE_DIAGNOSTIC=1'])]:
        binary = base / mode
        subprocess.run([cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                        '-DPIONEER_FFN_NATIVE_ROUTING_INCLUDE="' + str(include) + '"', *flags,
                        str(directory / 'pioneer_q38_ffn_code_native_routing_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    source = '#include <cstdint>\n#define B_DEVICE_OP_CODES_END 0x1000\n#include "' + str(root / 'src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h') + '"\n'
    cases = 0
    for role in ['PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT']:
        other = 'PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT' if role.endswith('DIAGNOSTIC') else 'PIONEER_Q38_FFN_CODE_DIAGNOSTIC'
        flags = [[f'-D{role}={bad}'] for bad in ['0', '2', '-1']]
        flags += [[f'-D{role}=1', f'-D{mixed}=1'] for mixed in [other, 'PIONEER_Q38_PEER_PHASE_QUALIFICATION',
                   'PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT', 'PIONEER_Q38_NATIVE_Q8_LAYOUT',
                   'PIONEER_Q38_NATIVE_Q8_QUALIFICATION', 'PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT']]
        for variant in flags:
            r = subprocess.run([cxx, '-std=c++17', '-fsyntax-only', *variant, '-x', 'c++', '-'], input=source, capture_output=True, text=True)
            assert r.returncode != 0 and 'Held-only FFN code qualification has its own exact isolated protocol role' in r.stderr
            cases += 1
    print(f'PASS {cases} actual protocol malformed/mixed-role compile refusals;NO_NATIVE_ACCESS')
