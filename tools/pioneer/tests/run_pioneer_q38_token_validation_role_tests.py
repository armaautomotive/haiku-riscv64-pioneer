#!/usr/bin/env python3
"""Compile-only exact physical validation role;no link/install/device access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
kernel = pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
native = kernel/'PioneerQ38TokenNativeRole.h'
physical = kernel/'PioneerQ38TokenValidationRole.h'
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
old = set(re.findall(pattern, native.read_text()))
required = set(re.findall(pattern, physical.read_text()))
assert len(old) == 21 and required == (old-{'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'}) | {
    'PIONEER_Q38_TOKEN_VALIDATION_NATIVE', 'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
assert len(required) == 23
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
role = physical.read_text()
aliases = role[role.index('#if defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY)'):role.index('#if defined(api_version)')]
forbidden = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', aliases))
assert not required & forbidden
# Every actual token host-fixture selector must be refused, including =0.
# This checks coverage against the fixture sources, not an arbitrary count.
fixtures = set()
for path in (pathlib.Path(__file__).resolve().parent).glob('pioneer_q38_token*'):
    if path.suffix in ('.h', '.cpp', '.inc'):
        fixtures.update(re.findall(r'^#(?:ifdef|ifndef|define)\s+(PIONEER_Q38_TOKEN_NATIVE_[A-Z0-9_]*FIXTURE)\b',
                                   path.read_text(), re.M))
assert fixtures and fixtures <= forbidden, fixtures-forbidden
assert {'PIONEER_Q38_NATIVE_PUSHED_FABRIC_TEST', 'PIONEER_Q38_TEST_CANCELLABLE_ROOT',
        'PIONEER_Q38_TEST_OBSERVED_TRACE', '__FAST_MATH__'} <= forbidden
base = {**dict.fromkeys(required, '1'), 'PIONEER_DRIVER_BUILD_TAG': '0x2026103000010074ULL'}
positive = negative = 0
def definitions(values):
    return ''.join(f'#define {name} {value}\n' for name, value in sorted(values.items()))
def include(path):
    return f'#include "{path}"\n'
def check(values, expected, header=native, tail=''):
    global positive, negative
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
        input=definitions(values)+include(header)+tail, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == expected, (values, tail, result.stderr)
    if expected:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1
for header in (native, physical):
    check(base, True, header, include(header))
    for name in sorted(required):
        for value in (None, '0', '2'):
            bad = base.copy()
            if value is None:
                del bad[name]
            else:
                bad[name] = value
            check(bad, False, header)
        check(base, False, header, f'#undef {name}\n#define {name} 0\n'+include(header))
    for name in (*forbidden, *abi):
        for value in ('0', '1'):
            check({**base, name: value}, False, header)
        check(base, False, header, f'#define {name} 0\n'+include(header))
    for tag in (None, '0', '0x51544b4e4f445256ULL', '0x5154324e4f445256ULL', '0x5154334e4f445256ULL',
                '0x2026102700010072ULL', '0x2026102800010073ULL', '0x2026103000010075ULL'):
        bad = base.copy()
        if tag is None:
            del bad['PIONEER_DRIVER_BUILD_TAG']
        else:
            bad['PIONEER_DRIVER_BUILD_TAG'] = tag
        check(bad, False, header)
old_base = {**dict.fromkeys(old, '1'), 'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT': '1',
            'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS': '1', 'PIONEER_DRIVER_BUILD_TAG': '0x5154334e4f445256ULL',
            **{name: 'old_token_'+name for name in abi}}
check(old_base, True)
for value in ('0', '1'):
    check({**old_base, 'PIONEER_Q38_TOKEN_VALIDATION_NATIVE': value}, False)
# Even a fully retagged/redefined late selection must not reuse the other
# role's already-selected native types. These are actual C++ static asserts.
to_physical = '#undef PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY\n'+''.join(f'#undef {name}\n' for name in abi)
to_physical += '#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026103000010074ULL\n#define PIONEER_Q38_TOKEN_VALIDATION_NATIVE 1\n'
check(old_base, False, tail=to_physical+include(native))
to_old = '#undef PIONEER_Q38_TOKEN_VALIDATION_NATIVE\n#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1\n'
to_old += '#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x5154334e4f445256ULL\n'
to_old += definitions({name: 'old_token_'+name for name in abi})
check(base, False, tail=to_old+include(native))
print(f'PASS token tag74 exact validation role/{positive} positives/{negative} missing+0+2+late+old-tag+renamed-ABI+CPU+Work+TEST+NODRV-transition refusals;compile ONLY,not module/runtime/physical/TPS proof')
