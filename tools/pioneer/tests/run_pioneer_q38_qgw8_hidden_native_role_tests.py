#!/usr/bin/env python3
"""Tag129 native and reload identity guards; syntax-only compilation."""
import os
from pathlib import Path
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
role = here.parent / 'PioneerQ38Qgw8HiddenNativeRole.h'
reload_header = here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
selector = 'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129'
names = (selector, '_KERNEL_MODE', 'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
         'PIONEER_Q38_QGW8_NATIVE_INSPECTION', 'PIONEER_Q38_HYBRID_FULL_NATIVE',
         'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_DRIVER_RELOAD')
base = dict.fromkeys(names, '1')
base['PIONEER_DRIVER_BUILD_TAG'] = '0x2026110700010129ULL'
forbidden = ('PIONEER_Q38_QGW8_HIDDEN_CLIENT_129', 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
             'PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION',
             'PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC', 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE',
             'PIONEER_Q38_TOKEN_VALIDATION_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '__FAST_MATH__')
count = 0
def check(header, values, okay, tail=''):
    global count
    text = ''.join('#define ' + k + ' ' + v + '\n' for k, v in sorted(values.items()))
    text += '#include "' + str(header) + '"\n' + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                            input=text, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    count += 1
check(role, {}, True)
check(role, base, True)
for key in (*names, 'PIONEER_DRIVER_BUILD_TAG'):
    for value in (None, '0', '2'):
        if key == selector and value is None:
            continue
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(role, bad, False)
for key in forbidden:
    for value in ('0', '1'):
        check(role, {**base, key: value}, False)
include = '#include "' + str(role) + '"\n'
check(role, base, False, '#undef ' + selector + '\n' + include)
check(role, {}, False, '#define ' + selector + ' 1\n' + include)
reload = {'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_Q38_HYBRID_FULL_NATIVE': '1',
          'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128': '1', selector: '1',
          'PIONEER_DRIVER_BUILD_TAG': '0x2026110700010129ULL'}
check(reload_header, reload, True)
for key in reload:
    bad = reload.copy();del bad[key];check(reload_header, bad, False)
for tag in ('0x2026110700010128ULL', '0x2026110700010127ULL', '0'):
    check(reload_header, {**reload, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
print(f'PASS tag129 native/reload role {count} syntax cases; no device, Work or model')
