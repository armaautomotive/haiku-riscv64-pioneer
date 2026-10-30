#!/usr/bin/env python3
"""Actual selected DATA and private G3 native perimeter; never device IO."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
role_path = here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h'
role = role_path.read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
base = {**dict.fromkeys(parents, '1'), 'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL',
        'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT':'1', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':'1',
        **{name:'pioneer_q38_token_context_body_'+name for name in abi}}

def include(path):
    return '#include "'+str(path)+'"\n'

role_include = include(role_path)
selected = include(here.parent/'PioneerQ38TokenSelectedPrograms.h')
footprint = include(here.parent/'PioneerQ38TokenFootprint.h')
worker = include(here.parent/'PioneerQ38SelectedGlobalWorker.h')
legacy = include(here.parent/'PioneerQ38TokenPrograms.h')
accepted = refused = 0

def check(source, okay):
    global accepted, refused
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (okay, source, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1

def definitions(values):
    return ''.join('#define '+name+' '+value+'\n' for name, value in sorted(values.items()))

prefix = definitions(base)
check(prefix+role_include+role_include+'static_assert(PioneerQ38TokenNativeProfile==3);\n', True)
check(prefix+role_include+selected+worker+selected+footprint+role_include+
      'static_assert(PioneerQ38TokenPrograms::Family==0x33505451);\n'+
      'static_assert(PioneerQ38TokenL1::SavedBytes==53913832);\n'+
      'static_assert(PioneerQ38SelectedWorkerPushed==1 && PioneerQ38SelectedWorkerBootEnd==55);\n', True)
for name in sorted(parents | {'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}):
    missing = dict(base)
    del missing[name]
    check(definitions(missing)+role_include, False)
    for value in ('0', '2'):
        check(definitions({**base, name:value})+role_include, False)
        check(prefix+role_include+'#undef '+name+'\n#define '+name+' '+value+'\n'+role_include, False)
for name in abi:
    missing = dict(base)
    del missing[name]
    check(definitions(missing)+role_include, False)
for tag in ('0', '0x51544b4e4f445256ULL', '0x5154324e4f445256ULL', '0x2026102700010072ULL'):
    check(definitions({**base, 'PIONEER_DRIVER_BUILD_TAG':tag})+role_include, False)
for name in ('PIONEER_Q38_TOKEN_PRECISE_PROGRAMS', 'PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
             'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION', 'PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    for value in ('0', '1'):
        check(definitions({**base, name:value})+role_include, False)
        check(prefix+role_include+'#define '+name+' '+value+'\n'+role_include, False)
# DATA selection can be tested without pretending to own a native context.
data = '#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1\n'
check(data+selected+worker+selected, True)
for value in ('0', '2'):
    check('#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS '+value+'\n'+selected+worker, False)
for header in (selected, footprint, worker):
    check(header+data+header, False)
    check(data+header+'#undef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS\n'+header, False)
    check(data+header+'#define PIONEER_Q38_TOKEN_PRECISE_PROGRAMS 1\n'+header, False)
check(data+legacy, False)
check(legacy+prefix+role_include, False)
check(footprint+prefix+role_include, False)
for header, valid in (
        ('PioneerQ38GlobalChainIsolation.h', {}),
        ('PioneerQ38GlobalWorkerImageRole.h', {'PIONEER_Q38_GLOBAL_WORKER_IMAGE':'1'}),
        ('PioneerQ38PreciseWorkerImageRole.h', {'PIONEER_Q38_PRECISE_WORKER_IMAGE':'1'}),
        ('PioneerQ38PushedWorkerImageRole.h', {'PIONEER_Q38_PRECISE_WORKER_IMAGE':'1', 'PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE':'1'})):
    image = include(here.parent/header)
    valid_prefix = definitions(valid)
    check(valid_prefix+image, True)
    for name in ('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS', 'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT'):
        for value in ('0', '1'):
            contamination = '#define '+name+' '+value+'\n'
            check(valid_prefix+contamination+image, False)
            check(valid_prefix+image+contamination+image, False)
# Exercise the actual driver's reserved-tag witness fragment, not a copy.
driver = (role_path.parent/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK', 1)[0]
def check_driver(values, okay):
    source = definitions(values)+driver
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-I'+str(role_path.parent), '-E', '-x', 'c++', '-'],
                            input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, result.stderr
check_driver(base, True)
check_driver({'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL'}, False)
for value in ('0', '1'):
    check_driver({**base, 'PIONEER_Q38_TOKEN_NATIVE_ROLE_TAG_OK':value}, False)
print(f'PASS G3 selected actual DATA/private native binding/positive{accepted}/refusals{refused}/ALL21 parents+2 profile selectors+6 ABI names/4 actual driver-prefix controls;COMPILE ONLY,NO lifecycle or launch grant')
