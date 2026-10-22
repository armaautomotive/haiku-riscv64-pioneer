#!/usr/bin/env python3
"""Isolated CPU-timer tag37,unchanged device/data ABI;NO SDK/device."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
native = (directory / 'build_qwen38_ffn_native_graph.sh').read_text()
assert 'ffn_task_tag=2026102200010037' in native and 'ffn_task_profile=1' in native
role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', native))
role.update(PIONEER_DRIVER_BUILD_TAG='2026102200010037', PIONEER_Q38_FFN_HOST_PROFILE='1')
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-host-profile-gates-') as temporary:
    base = pathlib.Path(temporary)
    def refuse(name, env):
        global cases
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-output-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output)]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **env}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout+result.stderr, (name, env, result)
        cases += 1
    for key in role:
        refuse('build_qwen38_banks_driver.sh', {**role, key: '0'})
    for value in ['01', '2', '-1', 'bad']:
        refuse('build_qwen38_banks_driver.sh', {**role, 'PIONEER_Q38_FFN_HOST_PROFILE': value})
    for key in ['PIONEER_Q38_FFN_HOST_PROFILE_CLIENT', 'PIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT',
                'PIONEER_Q38_FFN_MODEL_INIT_CLIENT', 'PIONEER_Q38_FFN_CODE_DIAGNOSTIC',
                'PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC',
                'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION', 'PIONEER_Q38_HOST_PHASE_PROFILE']:
        refuse('build_qwen38_ffn_host_profile.sh', {key: '1'})
    for name in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_inference.sh',
                 'build_qwen38_ffn_native_graph.sh', 'build_qwen38_ffn_native_graph_client.sh',
                 'build_qwen38_ffn_model_init.sh', 'build_qwen38_ffn_model_init_client.sh',
                 'build_qwen38_ffn_prime.sh', 'build_qwen38_ffn_host_profile_client.sh']:
        for key in ['PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_HOST_PROFILE_CLIENT']:
            for value in ['1', '01', '-1', '2', 'bad']:
                refuse(name, {key: value})
    # Profile is CPU-only:device images/catalog and public graph request
    # remain the exact qualified versions. Compile actual headers both ways.
    source = base / 'namespace.cpp'
    headers = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
    source.write_text('#include <cstddef>\n#define B_DEVICE_OP_CODES_END 1000\n' +
                      '\n'.join('#include "' + str(headers / name) + '"' for name in
                                ['PioneerQ38WorkProtocol.h', 'PioneerQ38FfnGraphProtocol.h']) +
                      '\nstatic_assert(TT_Q38_WORK_SUPPORT_VERSION==9);\n'
                      'static_assert(TT_PCI_PROBE_Q38_FFN_GRAPH_RUN==1000+0x62b1);\n'
                      'static_assert(sizeof(tt_q38_ffn_graph_run)==82000);\n')
    for side in ['', '_CLIENT']:
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only',
                        '-DPIONEER_Q38_FFN_NATIVE_GRAPH' + side + '=1', '-DPIONEER_Q38_FFN_MODEL_INIT' + side + '=1',
                        '-DPIONEER_Q38_FFN_HOST_PROFILE' + side + '=1', str(source)], check=True)
print(f'PASS {cases} host-profile exact-role/before-SDK/old-role/Assistant refusals;namespace9+82000byte public ABI unchanged;NO device')
