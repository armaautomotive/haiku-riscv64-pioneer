#!/usr/bin/env python3
"""Run actual full-offload grammar/session on synthetic transport,never hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert sys.argv[1:] in ([], ['--batched'], ['--batched','--metrics'])
batched = bool(sys.argv[1:])
metrics = '--metrics' in sys.argv
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-full-offload-tests-',dir='/private/tmp') as temporary:
    role=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h').read_text()
    required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
    assert len(required)==23
    (pathlib.Path(temporary)/'full_offload_role_defines.inc').write_text(
        '\n'.join('#define '+name+' 1' for name in sorted(required))
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1' if batched else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1' if metrics else '')
        + '\n#define PIONEER_DRIVER_BUILD_TAG '+('0x2026110400010090ULL' if metrics else '0x2026110400010089ULL' if batched else '0x2026110400010088ULL')+'\n')
    for name in ('protocol','session','control'):
        source=here/('pioneer_q38_full_offload_'+name+'_test.cpp')
        binary=pathlib.Path(temporary)/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1'] if batched else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1'] if metrics else []),
                        '-I'+temporary,str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS full-offload host tests;no original model,native engine,device or speed result')
