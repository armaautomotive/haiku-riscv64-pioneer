#!/usr/bin/env python3
"""Standalone QRN1 is not a native/old-image selector;before-SDK refusals."""
import os
import pathlib
import subprocess
import tempfile
here = pathlib.Path(__file__).resolve().parents[1]
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-learned-rms-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for flag in ('PIONEER_Q38_LEARNED_RMS', 'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE',
                 'PIONEER_Q38_PROJECTION_GRAPH_ROOT', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '01', '', '-1', '0 ', '00', '0\n1'):
            result = subprocess.run(['sh', str(here / 'build_qwen38_learned_rms_image.sh'), str(sdk), str(output)],
                                    env={**clean, flag: value}, capture_output=True, text=True)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
            count += 1
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for flag in ('PIONEER_Q38_LEARNED_RMS', 'PIONEER_Q38_LEARNED_RMS_NATIVE'):
            for value in ('1', '2', '01', '', '-1', '0 ', '00', '0\n1'):
                result = subprocess.run(['sh', str(here / ('build_qwen38_' + name + '.sh')), str(sdk), str(output)],
                                        env={**clean, flag: value}, capture_output=True, text=True)
                assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
                count += 1
    for kind in ('learned_rms', 'rms'):
        result = subprocess.run(['sh', str(here / 'build_qwen38_local_image.sh'), kind, str(sdk), str(output)],
                                env=clean, capture_output=True, text=True)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
        count += 1
print(f'PASS learned RMS build gates{count};new/old roles refused BEFORE SDK/output,old image registry unchanged;NO native selection/install')
