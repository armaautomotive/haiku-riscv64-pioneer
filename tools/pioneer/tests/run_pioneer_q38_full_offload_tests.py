#!/usr/bin/env python3
"""Run actual full-offload grammar/session on synthetic transport,never hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

args=sys.argv[1:]
resident='--resident' in args
if resident:
    assert args.count('--resident')==1 and '--batched' in args and '--metrics' in args
    args.remove('--resident')
fixed_config='--fixed-config' in args
if fixed_config:
    assert args[-1]=='--fixed-config' and args[:-1]==['--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block']
    args=args[:-1]
assert args in ([], ['--batched'], ['--batched','--metrics'], ['--batched','--metrics','--vector-q4'], ['--batched','--metrics','--prepared-q4'], ['--batched','--metrics','--prepared-q4','--counter-unroll'], ['--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile'], ['--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile','--two-product-replay'], ['--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch'], ['--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block'])
batched = bool(sys.argv[1:])
metrics = '--metrics' in sys.argv
vector_q4 = '--vector-q4' in sys.argv
prepared_q4 = '--prepared-q4' in sys.argv
counter_unroll = '--counter-unroll' in sys.argv
phase_profile = '--phase-profile' in sys.argv
two_product_replay = '--two-product-replay' in sys.argv
instruction_prefetch = '--instruction-prefetch' in sys.argv
queued_block = '--queued-block' in sys.argv
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
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL 1' if counter_unroll else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE 1' if phase_profile else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY 1' if two_product_replay else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH 1' if instruction_prefetch else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4 1' if queued_block else '')
        + ('\n#define PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4 1' if fixed_config else '')
        + '\n#define PIONEER_DRIVER_BUILD_TAG '+('0x2026110500010103ULL' if fixed_config else '0x2026110500010102ULL' if queued_block else '0x2026110500010101ULL' if instruction_prefetch else '0x2026110500010100ULL' if two_product_replay else '0x2026110500010099ULL' if phase_profile else '0x2026110500010098ULL' if counter_unroll else '0x2026110500010097ULL' if prepared_q4 else '0x2026110400010092ULL' if vector_q4 else '0x2026110400010090ULL' if metrics else '0x2026110400010089ULL' if batched else '0x2026110400010088ULL')+'\n')
    for name in (('protocol','session') if resident else ('protocol','session','control')):
        source=here/('pioneer_q38_full_offload_'+name+'_test.cpp')
        binary=pathlib.Path(temporary)/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1'] if batched else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1'] if metrics else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=1'] if resident else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1'] if resident else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1'] if vector_q4 else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1'] if prepared_q4 else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL=1'] if counter_unroll else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE=1'] if phase_profile else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY=1'] if two_product_replay else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH=1'] if instruction_prefetch else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4=1'] if queued_block else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4=1'] if fixed_config else []),
                        *(['-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6"'] if prepared_q4 else []),
                        *(['-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1'] if (vector_q4 or prepared_q4) and name!='control' else []),
                        '-I'+temporary,str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS full-offload host tests;no original model,native engine,device or speed result')
