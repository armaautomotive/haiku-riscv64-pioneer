#!/usr/bin/env python3
"""Native-token COMPILE-ONLY role checks; no runtime/driver/device access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
path = here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h'
role = path.read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
forbidden = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role[role.index('#if defined(PIONEER_Q38_EMBEDDING_NATIVE)'):]))
assert not parents & forbidden and len(forbidden) == 34
base = {**dict.fromkeys(parents, '1'), 'PIONEER_DRIVER_BUILD_TAG':'0x51544b4e4f445256ULL',
        **{name:'pioneer_q38_token_context_body_'+name for name in abi}}
include = '#include "'+str(path)+'"\n'


def check(definitions, accept, tail=''):
    source = ''.join('#define '+name+' '+value+'\n' for name, value in sorted(definitions.items()))+include+tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-E', '-x', 'c++', '-'],
                            input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == accept, (accept, result.stderr)


check(base, True)
refused = 0
for name in sorted(parents):
    missing = dict(base)
    del missing[name]
    check(missing, False)
    refused += 1
    for value in ('0', '2'):
        check({**base, name:value}, False)
        check(base, False, '#undef '+name+'\n#define '+name+' '+value+'\n'+include)
        refused += 2
for name in abi:
    missing = dict(base)
    del missing[name]
    check(missing, False)
    refused += 1
for tag in ('0', '0x5152504e4f445256ULL', '0x2026102700010072ULL'):
    check({**base, 'PIONEER_DRIVER_BUILD_TAG':tag}, False)
    refused += 1
for name in sorted(forbidden):
    for value in ('0', '1'):
        check({**base, name:value}, False)
        check(base, False, '#define '+name+' '+value+'\n'+include)
        refused += 2
# Also exercise the real driver's FIRST witness/tag block,not a parallel
# hand-written guard. No SDK/kernel/device headers follow this fragment.
driver = (path.parent/'tenstorrent_pci_probe.cpp').read_text()
marker = '#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK'
assert driver.count(marker) == 1
prefix = driver.split(marker, 1)[0]


def driver_prefix(definitions, accept, diagnostic=''):
    source = ''.join('#define '+name+' '+value+'\n' for name, value in sorted(definitions.items()))+prefix
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-I'+str(path.parent),
                             '-E', '-x', 'c++', '-'], input=source, text=True,
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == accept and diagnostic in result.stderr, result.stderr


driver_prefix(base, True)
for value in ('0', '1'):
    driver_prefix({**base, 'PIONEER_Q38_TOKEN_NATIVE_ROLE_TAG_OK':value}, False,
                  'Internal token NODRV witness cannot be supplied')
    refused += 1
missing = dict(base)
del missing['PIONEER_Q38_TOKEN_NATIVE_CONTEXT']
driver_prefix(missing, False, 'Reserved QTKNODRV tag requires its exact')
refused += 1
# Fixed program DATA is deliberately still QTP1/QGW1. Do not let the
# new candidate's DATA selection silently pair QGW2 boot words with those
# old ROMs and their old peer journal, even outside a native compile TU.
programs = '#include "'+str(here.parent/'PioneerQ38TokenPrograms.h')+'"\n'
for candidate in ('PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION', 'PIONEER_Q38_PRECISE_WORKER_IMAGE'):
    for value in ('0', '1'):
        for late in (False, True):
            define = '#define '+candidate+' '+value+'\n'
            source = (programs+define+programs) if late else define+programs
            result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', '-'],
                                    input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            assert result.returncode and 'Fixed QTP1/QGW1 program DATA cannot select' in result.stderr, result.stderr
            refused += 1
print(f'PASS native QTKNODRV compile role/21 exact parents/6 renamed ABI names/{len(forbidden)} old-native+image+CPU+Work aliases/{refused} undefined+zero+two+late refusals;NO link/install/runtime/physical authority')
