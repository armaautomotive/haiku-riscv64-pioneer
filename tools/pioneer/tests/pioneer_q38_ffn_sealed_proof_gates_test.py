#!/usr/bin/env python3
"""Exact measured tag38 ONLY;old roles/Assistant inert before SDK access."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
native = (directory / 'build_qwen38_ffn_native_graph.sh').read_text()
assert 'ffn_task_sealed=1;ffn_task_tag=2026102200010038' in native
role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', native))
role.update(PIONEER_DRIVER_BUILD_TAG='2026102200010038', PIONEER_Q38_FFN_HOST_PROFILE='1',
            PIONEER_Q38_FFN_SEALED_PROOF='1')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-sealed-gates-') as directory_name:
    base = pathlib.Path(directory_name)
    def refuse(name, env):
        global cases
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output)]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **env}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout+result.stderr, (name, env, result)
        cases += 1
    for key in role:
        refuse('build_qwen38_banks_driver.sh', {**role, key: '0'})
    for value in ['01', '2', '-1', 'bad']:
        refuse('build_qwen38_banks_driver.sh', {**role, 'PIONEER_Q38_FFN_SEALED_PROOF': value})
    for tag in ['2026102200010036', '2026102200010037', '2026102200010039']:
        refuse('build_qwen38_banks_driver.sh', {**role, 'PIONEER_DRIVER_BUILD_TAG': tag})
    for key in ['PIONEER_Q38_FFN_SEALED_PROOF_CLIENT', 'PIONEER_Q38_FFN_HOST_PROFILE_CLIENT',
                'PIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT', 'PIONEER_Q38_FFN_MODEL_INIT_CLIENT',
                'PIONEER_Q38_FFN_CODE_DIAGNOSTIC', 'PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC',
                'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC', 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION']:
        refuse('build_qwen38_ffn_sealed_proof.sh', {key: '1'})
    for name in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_inference.sh',
                 'build_qwen38_ffn_native_graph.sh', 'build_qwen38_ffn_native_graph_client.sh',
                 'build_qwen38_ffn_model_init.sh', 'build_qwen38_ffn_model_init_client.sh',
                 'build_qwen38_ffn_host_profile.sh', 'build_qwen38_ffn_host_profile_client.sh',
                 'build_qwen38_ffn_prime.sh', 'build_qwen38_ffn_sealed_proof_client.sh']:
        for key in ['PIONEER_Q38_FFN_SEALED_PROOF', 'PIONEER_Q38_FFN_SEALED_PROOF_CLIENT']:
            for value in ['1', '01', '-1', '2', 'bad']:
                refuse(name, {key: value})
print(f'PASS {cases} sealed-proof exact tag38/before-SDK/old-role/Assistant refusals;NO device')
