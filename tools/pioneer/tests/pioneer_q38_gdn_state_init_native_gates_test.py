#!/usr/bin/env python3
"""Actual tag47 scope isolation/old-builder refusal/zero public authority."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
valid = dict(PIONEER_Q38_GDN_STATE_INIT_NATIVE='1', PIONEER_Q38_GDN_INPUT_OWNER='1', PIONEER_Q38_RESIDENT_DECODER_NATIVE='1',
             PIONEER_Q38_RESIDENT_DECODER_OWNER='1', PIONEER_Q38_DECODER_RUNTIME='1', PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1',
             PIONEER_Q38_PROJECTION_RESULT_STASH='1', PIONEER_Q38_LEARNED_RMS_NATIVE='1', PIONEER_Q38_EMBEDDING_NATIVE='1',
             TT_PIONEER_DRIVER_RELOAD='1', PIONEER_DRIVER_BUILD_TAG='0x2026102400010047ULL')
count = 0
def preprocess(flags, expected):
    global count
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                            input=prefix, capture_output=True, text=True, env=clean)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1
preprocess({}, True)
preprocess(valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        changed = dict(valid)
        if value is None:
            del changed[key]
        else:
            changed[key] = value
        preprocess(changed, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010042ULL', '0x2026102300010043ULL',
            '0x2026102300010044ULL', '0x2026102300010045ULL', '0x2026102300010046ULL'):
    preprocess({**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess({'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
             'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION',
             'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess({**valid, flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-state-init-gates-') as temporary:
    base = pathlib.Path(temporary)
    sdk, output = base / 'MUST_NOT_READ_SDK', base / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = here.parent / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_GDN_STATE_INIT_NATIVE' in line]
        assert len(guards) == 1
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_GDN_STATE_INIT_NATIVE': value}
            subprocess.run(['sh', '-c', 'set -eu\n' + guards[0]], check=True, env=env)
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            result = subprocess.run(['sh', str(script), str(sdk), str(output)], capture_output=True, text=True, timeout=5,
                                    env={**clean, 'PIONEER_Q38_GDN_STATE_INIT_NATIVE': value})
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    for name in ('pioneer_q38_gdn_state_init_native_full_compile.py', 'pioneer_q38_gdn_input_native_full_compile.py'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(['python3', str(here / name), str(sdk), str(output)], capture_output=True, text=True, timeout=5,
                                    env={**clean, 'PIONEER_Q38_GDN_STATE_INIT_NATIVE': value})
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    begin, end = '// PIONEER_Q38_GDN_STATE_INIT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_STATE_INIT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0].strip()
    assert route == 'return B_NOT_ALLOWED; // COMPILE-ONLY tag47 has NO public zero/compute/upload/old Work selector'
    start = 'static status_t\nProbeControl(void* cookie, uint32 operation, void* data, size_t length)\n{'
    prelude = start + source.split(start, 1)[1].split('#ifdef TT_PIONEER_DRIVER_RELOAD', 1)[0]
    prelude = prelude.replace('{\n', '{\n (void)operation;(void)data;(void)length;\n', 1)
    code = '#include <cassert>\n#include <cstddef>\nusing uint32=unsigned;using status_t=int;\n' \
           'int uid=0;int geteuid(){return uid;}enum{B_NOT_ALLOWED=-1};struct PioneerProbeCookie{unsigned magic;};\n' \
           + prelude + route + '\n}\nint main(){PioneerProbeCookie c={0x50313030};\n' \
           'for(unsigned op=0;op<131072;op++)assert(ProbeControl(&c,op,nullptr,0)==B_NOT_ALLOWED);\n' \
           'assert(ProbeControl(nullptr,0,nullptr,0)==B_NOT_ALLOWED);uid=1;assert(ProbeControl(&c,0,nullptr,0)==B_NOT_ALLOWED);}\n'
    binary = base / 'routing'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-', '-o', str(binary)],
                   input=code, env=clean, text=True, check=True)
    subprocess.run([str(binary)], env=clean, check=True)
    count += 1
print(f'PASS ACTUAL tag47 gates{count}/exact SAME-native-owner only/old tags+mixed DEBUG-CPUref-Work refused/legacy builders inert BEFORE SDK-output/131072 public-route refusals;NO public zero or compute selector/device access')
