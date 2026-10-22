#!/usr/bin/env python3
"""Exact isolated native role;no old-role/Assistant promotion. NO device."""
import os
import pathlib
import re
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-persistent-graph-gates-') as temporary:
    root = pathlib.Path(temporary)
    for name in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_inference.sh',
                 'build_qwen38_ffn_model_init.sh', 'build_qwen38_ffn_model_init_client.sh', 'build_qwen38_ffn_prime.sh',
                 'build_qwen38_ffn_root_lifetime_diagnostic.sh']:
        for value in ['1', '01', '-1', '2', 'bad']:
            sdk, output = root / 'MUST_NOT_OPEN_SDK', root / ('never-output-' + str(cases))
            args = ['sh', str(directory / name), str(sdk), str(output)]
            if name == 'build_qwen38_inference.sh':
                args.append(str(root / 'MUST_NOT_OPEN_LIBRARIES'))
            r = subprocess.run(args, env={**clean, 'PIONEER_Q38_FFN_PERSISTENT_GRAPH': value}, capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not output.exists() and str(sdk) not in r.stdout+r.stderr, (name, value, r)
            cases += 1
    native_script = (directory / 'build_qwen38_ffn_native_graph.sh').read_text()
    role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', native_script))
    assert 'ffn_task_tag=2026102200010036' in native_script
    role['PIONEER_DRIVER_BUILD_TAG'] = '2026102200010036'
    assert role['PIONEER_DRIVER_BUILD_TAG'] == '2026102200010036' and role['PIONEER_Q38_FFN_NATIVE_GRAPH'] == '1'

    def refuse(name, overrides):
        global cases
        sdk, output = root / 'MUST_NOT_OPEN_SDK', root / ('never-output-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output)]
        if name == 'build_qwen38_inference.sh':
            args.append(str(root / 'MUST_NOT_OPEN_LIBRARIES'))
        r = subprocess.run(args, env={**clean, **overrides}, capture_output=True, text=True, timeout=5)
        assert r.returncode == 2 and not output.exists() and str(sdk) not in r.stdout+r.stderr, (name, overrides, r)
        cases += 1

    for key in role:
        refuse('build_qwen38_banks_driver.sh', {**role, key: '0'})
    for key in ['PIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT', 'PIONEER_Q38_FFN_MODEL_INIT_CLIENT',
                'PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC',
                'PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC',
                'PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC', 'PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC',
                'PIONEER_Q38_GDN_MUTABLE_STATE', 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION', 'PIONEER_Q38_DISPATCH_PROFILE']:
        refuse('build_qwen38_ffn_native_graph.sh', {key: '1'})
    for name in ['build_qwen38_banks_client.sh', 'build_qwen38_inference.sh', 'build_qwen38_ffn_prime.sh',
                 'build_qwen38_ffn_model_init.sh', 'build_qwen38_ffn_model_init_client.sh',
                 'build_qwen38_ffn_native_graph_client.sh']:
        for key in ['PIONEER_Q38_FFN_NATIVE_GRAPH', 'PIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT']:
            for value in ['1', '01', '-1', '2', 'bad']:
                refuse(name, {key: value})
    source = root / 'namespace.cpp'
    headers = directory.parents[1] / 'src/add-ons/kernel/drivers/misc'
    source.write_text('#include <cstddef>\n#define B_DEVICE_OP_CODES_END 1000\n' +
                      '\n'.join('#include "' + str(headers / name) + '"' for name in
                                ['PioneerQ38WorkProtocol.h', 'PioneerQ38FfnModelProtocol.h', 'PioneerQ38FfnGraphProtocol.h']) +
                      '\nstatic_assert(TT_Q38_WORK_SUPPORT_VERSION==9);\n'
                      'static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT==1000+0x62b4);\n'
                      'static_assert(TT_PCI_PROBE_Q38_WORK_RUN==1000+0x62b5);\n'
                      'static_assert(TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT==1000+0x62b6);\n'
                      'static_assert(TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE==1000+0x62b7);\n'
                      'static_assert(TT_PCI_PROBE_Q38_FFN_GRAPH_RUN==1000+0x62b1);\n')
    for side in ['', '_CLIENT']:
        command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only',
                   '-DPIONEER_Q38_FFN_NATIVE_GRAPH' + side + '=1', '-DPIONEER_Q38_FFN_MODEL_INIT' + side + '=1', str(source)]
        subprocess.run(command, check=True)
        for mix in ['PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_NATIVE_Q8_LAYOUT', 'PIONEER_Q38_PEER_PHASE_QUALIFICATION']:
            r = subprocess.run(command[:-1] + ['-D' + mix + '=1', str(source)], capture_output=True, text=True)
            assert r.returncode != 0
            cases += 1
print(f'PASS {cases} persistent FFN exact-tag36 role/before-SDK/old-role/Assistant refusals plus actual namespace9 header checks;NO device access')
