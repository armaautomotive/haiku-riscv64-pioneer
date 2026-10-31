#!/usr/bin/env python3
"""Actual routing source, stub handlers only: no native/device execution."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
start = source.index('static status_t\nProbeControl(')
branch = source.index('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT', start)
head = source[start:branch]
assert 'geteuid() != 0' in head and 'PioneerQ38MixedLiveNativeRouting.inc' in head
selected = []
depth = 0
for line in source[branch:].splitlines(keepends=True):
    text = line.lstrip()
    if text.startswith(('#if ', '#if\t', '#ifdef ', '#ifndef ')):
        depth += 1
    selected.append(line)
    if text.startswith('#endif'):
        depth -= 1
        if not depth:
            break
assert not depth and 'Q38TokenNativeControl(instance,data,length)' in ''.join(selected)
with tempfile.TemporaryDirectory(prefix='pioneer-token-validation-routing-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    (out/'token_actual_routing.inc').write_text(head+''.join(selected)+'\n}\n')
    for physical in (0, 1, 2):
        binary = out/('test'+str(physical))
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-DTOKEN_ROUTING_PHYSICAL='+str(physical), '-I'+str(out),
                        '-I'+str(kernel), str(here/'pioneer_q38_token_validation_routing_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
