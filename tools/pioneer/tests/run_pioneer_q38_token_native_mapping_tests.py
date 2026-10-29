#!/usr/bin/env python3
"""Production full-token L1 mapping; OS/scope endpoints simulated ONLY."""
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
defines.update(PIONEER_DRIVER_BUILD_TAG='0x51544b4e4f445256ULL',TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1',TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
defines.update({name:'pioneer_token_mapping_test_'+name for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-map-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    str(here/'pioneer_q38_token_native_mapping_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
