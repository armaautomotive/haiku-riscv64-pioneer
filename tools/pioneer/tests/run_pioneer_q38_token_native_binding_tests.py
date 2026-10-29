#!/usr/bin/env python3
"""Actual full851 native bind/scope/retention; simulated OS/unused endpoints."""
import os
import pathlib
import re
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
defines.update({name:'pioneer_token_binding_test_'+name for name in
                ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')})
defines.update(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1', TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-binding-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    # Exact WHOLE production Unsafe method,not an implementation of just
    # the new controlledToken exception which hides unrelated unsafe owners.
    start, end = '\tbool Unsafe(\n', '\n\tvoid Snapshot(tt_resident_session_result& r)'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_unsafe.inc').write_text(start+source.split(start, 1)[1].split(end, 1)[0])
    start, end = '// PIONEER_PROBE_LOCK_BEGIN', '// PIONEER_PROBE_LOCK_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_lock.inc').write_text(source.split(start, 1)[1].split(end, 1)[0])
    start, end = '// PIONEER_Q38_RETAIN_MEMORY_BEGIN', '// PIONEER_Q38_RETAIN_MEMORY_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_retain.inc').write_text(source.split(start, 1)[1].split(end, 1)[0])
    # Actual FIRST full-token branch of END. No later legacy cleanup is
    # silently provided by the fixture or treated as a new-token capability.
    start = 'EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)\n{\n'
    assert source.count(start) == 1
    branch = source.split(start, 1)[1].split('\n#endif', 1)[0]+'\n#endif\n'
    assert branch.startswith('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT\n')
    (base/'token_native_end.inc').write_text(branch)
    binary = base/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), str(here/'pioneer_q38_token_native_binding_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
