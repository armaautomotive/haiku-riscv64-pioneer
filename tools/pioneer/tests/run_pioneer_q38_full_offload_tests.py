#!/usr/bin/env python3
"""Run actual full-offload grammar/session on synthetic transport,never hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert sys.argv[1:] in ([], ['--batched'], ['--batched','--metrics'], ['--batched','--metrics','--vector-q4'], ['--batched','--metrics','--prepared-q4'])
batched = bool(sys.argv[1:])
metrics = '--metrics' in sys.argv
vector_q4 = '--vector-q4' in sys.argv
prepared_q4 = '--prepared-q4' in sys.argv
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
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4 1' if vector_q4 else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 1' if prepared_q4 else '')
        + '\n#define PIONEER_DRIVER_BUILD_TAG '+('0x2026110500010097ULL' if prepared_q4 else '0x2026110400010092ULL' if vector_q4 else '0x2026110400010090ULL' if metrics else '0x2026110400010089ULL' if batched else '0x2026110400010088ULL')+'\n')
    for name in ('protocol','session','control'):
        source=here/('pioneer_q38_full_offload_'+name+'_test.cpp')
        binary=pathlib.Path(temporary)/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1'] if batched else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1'] if metrics else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1'] if vector_q4 else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1'] if prepared_q4 else []),
                        *(['-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6"'] if prepared_q4 else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1'] if (vector_q4 or prepared_q4) and name!='control' else []),
                        '-I'+temporary,str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS full-offload host tests;no original model,native engine,device or speed result')
