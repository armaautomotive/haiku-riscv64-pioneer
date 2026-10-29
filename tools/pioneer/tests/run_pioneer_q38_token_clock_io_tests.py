#!/usr/bin/env python3
"""Actual extracted native clock adapter,real strict-clock bodies;NO hardware."""
import os
import pathlib
import re
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
begin, end = '// PIONEER_Q38_CLOCK_IO_BEGIN', '// PIONEER_Q38_CLOCK_IO_END'
assert source.count(begin) == source.count(end) == 1
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
definitions = dict.fromkeys(parents, '1')
definitions['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
definitions.update({name:'pioneer_token_clock_io_test_'+name for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
with tempfile.TemporaryDirectory(prefix='pioneer-token-clock-io-', dir='/private/tmp') as temporary:
    base = pathlib.Path(temporary)
    (base/'token_native_clock_io.inc').write_text(source.split(begin,1)[1].split(end,1)[0])
    binary = base/'test'
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k,v in sorted(definitions.items())],
                    '-I'+str(base), '-I'+str(kernel), str(here/'pioneer_q38_token_clock_io_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
