#!/usr/bin/env python3
"""Private memory component MUST NOT leak into any old/native/image role."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-memory-gates-') as temporary:
    base = pathlib.Path(temporary)
    for name, owner in [('build_qwen38_banks_driver.sh', {}), ('build_qwen38_banks_client.sh', {}), ('build_qwen38_inference.sh', {}),
                        ('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1'}),
                        ('build_qwen38_ffn_matrix_image.sh', {'PIONEER_Q38_FFN_RESIDENT_MATRIX': '1'}),
                        ('build_qwen38_ffn_persistent_image.sh', {'PIONEER_Q38_FFN_PERSISTENT': '1'}),
                        ('build_qwen38_local_image.sh', {}), ('build_qwen38_projection_graph_image.sh', {}),
                        ('build_qwen38_projection_matrix_image.sh', {}), ('build_qwen38_projection_persistent_image.sh', {})]:
        for value in ['1', '01', '2', '-1', 'bad']:
            sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-' + str(cases))
            args = ['sh', str(directory / name), *(['projection_gather'] if name == 'build_qwen38_local_image.sh' else []), str(sdk), str(output)]
            if name == 'build_qwen38_inference.sh':
                args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
            result = subprocess.run(args, env={**clean, **owner, 'PIONEER_Q38_PROJECTION_GRAPH_MEMORY': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, value, result)
            cases += 1
print(f'PASS {cases} before-SDK projection memory refusals across old driver/client/Assistant and ALL root/peer/local image builders;NO_NATIVE_SELECTOR_OR_LAUNCH')
