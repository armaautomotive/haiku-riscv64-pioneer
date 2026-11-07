#!/usr/bin/env python3
"""Exact new code/geometry/roles; NO device access or execution authority."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image, out = map(Path, sys.argv[1:])
assert image.is_absolute() and image.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = kernel/'PioneerQ38FullOffloadRole.h'
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role.read_text()))
assert len(required) == 23
base = {name: '1' for name in required}
base.update(PIONEER_DRIVER_BUILD_TAG='0x2026110600010116ULL', PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
            PIONEER_Q38_FULL_OFFLOAD_METRICS='1', PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE='1')
definitions = lambda values: ''.join('#define '+k+' '+v+'\n' for k, v in sorted(values.items()))
include = lambda p: '#include "'+str(p)+'"\n'
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror']
generated = out/'programs.h'
subprocess.run([sys.executable, str(here.parent/'generate_qwen38_fast_decode_profile_programs.py'), str(image), str(generated)], check=True)
header = here.parent/'PioneerQ38FullOffloadFastDecodePrograms.h'
assert generated.read_bytes() == header.read_bytes()
(out/'fast_decode_native_role_defines.inc').write_text(definitions(base))
binary = out/'data-test'
subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', '-I'+str(out),
                str(here/'pioneer_q38_fast_decode_native_data_test.cpp'), '-o', str(binary)], check=True)
subprocess.run([str(binary), str(image)], check=True, env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
positive = negative = 0
def syntax(values, okay, body=None):
    global positive, negative
    source = definitions(values)+(body if body is not None else include(header)+include(header))
    result = subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1
syntax(base, True)
selected = here.parent/'PioneerQ38TokenSelectedPrograms.h'
syntax(base, True, include(selected)+include(selected)+'static_assert(PioneerQ38TokenPrograms::FastDecodeProfile==1);\n')
for key in sorted(base):
    for value in (None, '0'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        syntax(bad, False)
    syntax(base, False, include(header)+'#undef '+key+'\n'+include(header))
for tag in ('0x2026110400010092ULL', '0x2026110500010099ULL', '0x2026110600010110ULL', '0x2026110600010115ULL'):
    syntax({**base, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
for key in ('PIONEER_Q38_FULL_OFFLOAD_RESIDENT', 'PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4',
            'PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4', 'PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4',
            'PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE', 'PIONEER_Q38_TOKEN_BF16_PROGRAMS',
            'PIONEER_Q38_BF16_FULL_PROFILE', 'PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS',
            'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
            'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', '__FAST_MATH__'):
    for value in ('0', '1'):
        syntax({**base, key: value}, False)
        syntax(base, False, include(header)+'#define '+key+' '+value+'\n'+include(header))
for old in ('PioneerQ38FullOffloadVectorQ4Programs.h', 'PioneerQ38FullOffloadBatchedPrograms.h',
            'PioneerQ38FullOffloadPreparedProfileQ4Programs.h'):
    syntax(base, False, include(here.parent/old)+include(header))
    syntax(base, False, include(header)+include(here.parent/old)+include(selected))
user = {'PIONEER_Q38_FULL_OFFLOAD_CLIENT': '1', 'PIONEER_Q38_FULL_OFFLOAD_BATCHED': '1',
        'PIONEER_Q38_FULL_OFFLOAD_METRICS': '1', 'PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE': '1',
        'B_DEVICE_OP_CODES_END': '1000'}
protocol = kernel/'PioneerQ38FullOffloadProtocol.h'
body = '#include <cstdint>\n'+include(protocol)+'''static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110600010116));
static_assert(TT_PCI_PROBE_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE==1000+0x8807);
static_assert(PioneerQ38SelectedPhaseProfileData::Magic==0x33485051);
#ifdef TT_PCI_PROBE_Q38_FULL_OFFLOAD_PHASE_PROFILE
#error Old phase opcode must not be routed
#endif
'''
syntax(user, True, body)
for key in ('PIONEER_Q38_FULL_OFFLOAD_CLIENT', 'PIONEER_Q38_FULL_OFFLOAD_BATCHED',
            'PIONEER_Q38_FULL_OFFLOAD_METRICS', 'PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE'):
    for value in (None, '0', '2'):
        bad = user.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        syntax(bad, False, body)
data = here.parent/'PioneerQ38SelectedPhaseProfileData.h'
syntax({}, True, include(data))
syntax({'PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE': '1'}, False,
       include(data)+'#undef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE\n'+include(data))
syntax({}, False, include(data)+'#define PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE 1\n'+include(data))
print(f'PASS distinct fast decode native DATA/roles:{positive} positive/{negative} missing,wrong,late,old-package,CPU,Work refusals; NO hardware or TPS', flush=True)
