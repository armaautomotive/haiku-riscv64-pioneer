#!/usr/bin/env python3
"""Exact new role/namespace;refuse leaks BEFORE SDK/output. NO device."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', (directory / 'build_qwen38_ffn_model_init.sh').read_text()))
assert role['PIONEER_Q38_FFN_MODEL_INIT'] == '1' and role['PIONEER_DRIVER_BUILD_TAG'] == '2026102200010035'
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-model-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(name, env):
        global cases
        sdk = base / 'MUST_NOT_OPEN_SDK'
        output = base / ('never-output-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output)]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **env}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout+result.stderr, (name, env, result)
        cases += 1

    for key in role:
        refuse('build_qwen38_banks_driver.sh', {**role, key: '0'})
    for key in ['PIONEER_Q38_GDN_MUTABLE_STATE', 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC', 'PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC',
                'PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT',
                'PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC', 'PIONEER_Q38_DISPATCH_PROFILE',
                'PIONEER_Q38_FFN_MODEL_INIT_CLIENT', 'PIONEER_Q38_NATIVE_Q8_QUALIFICATION']:
        refuse('build_qwen38_banks_driver.sh', {**role, key: '1'})
    for value in ['01', '2', '-1', 'bad']:
        refuse('build_qwen38_banks_driver.sh', {**role, 'PIONEER_Q38_FFN_MODEL_INIT': value})
    for name in ['build_qwen38_banks_client.sh', 'build_qwen38_inference.sh', 'build_qwen38_ffn_prime.sh', 'build_qwen38_ffn_model_init_client.sh']:
        for key in ['PIONEER_Q38_FFN_MODEL_INIT', 'PIONEER_Q38_FFN_MODEL_INIT_CLIENT']:
            for value in ['1', '-1', 'bad']:
                refuse(name, {key: value})

    # Compile the actual namespace header with both exact sides. Neither
    # old Work/model clients nor an accidental mixed role share the ops.
    source = base / 'namespace.cpp'
    source.write_text('#include <cstddef>\n#define B_DEVICE_OP_CODES_END 1000\n#include "' +
                      str(here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h') +
                      '"\nstatic_assert(TT_Q38_WORK_SUPPORT_VERSION==8);\n'
                      'static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT==1000+0x62a2);\n'
                      'static_assert(TT_PCI_PROBE_Q38_WORK_RUN==1000+0x62a3);\n')
    for exact in ['PIONEER_Q38_FFN_MODEL_INIT', 'PIONEER_Q38_FFN_MODEL_INIT_CLIENT']:
        command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', '-D'+exact+'=1', str(source)]
        subprocess.run(command, check=True)
        for other in ['PIONEER_Q38_NATIVE_Q8_LAYOUT', 'PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_PEER_PHASE_QUALIFICATION']:
            result = subprocess.run(command[:-1]+['-D'+other+'=1', str(source)], capture_output=True, text=True)
            assert result.returncode != 0
            cases += 1
print(f'PASS {cases} canonical FFN model exact-role/mixed/production refusals BEFORE SDK/output plus namespace8 actual-header checks;NO native IO')
