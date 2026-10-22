#!/usr/bin/env python3
"""Before-SDK/output role refusals;NO build/device activity."""
import os
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parent.parent
environment = {key: value for key, value in os.environ.items()
               if not key.startswith(('PIONEER_', 'TT_PIONEER_'))}
sdk = '/DO_NOT_ACCESS_PIONEER_QFG2_TEST_SDK'
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-qfg2-gates-') as name:
    output = pathlib.Path(name) / 'NEVER_CREATED'

    def refuse(script, flags, arguments=2):
        global count
        args = [sdk, str(output)] + ([str(output) + '-libs'] if arguments == 3 else [])
        result = subprocess.run(['sh', str(root / script), *args],
                                env=dict(environment, **flags), text=True, capture_output=True)
        assert result.returncode == 2, (script, flags, result.returncode, result.stderr)
        assert sdk not in result.stdout + result.stderr and not output.exists(), (script, flags, result.stderr)
        count += 1

    for script in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh',
                   'build_qwen38_inference.sh', 'build_qwen38_dispatch_image.sh',
                   'build_qwen38_ffn_matrix_image.sh', 'build_qwen38_ffn_persistent_image.sh']:
        for value in ['1', '2', '-1', 'invalid']:
            refuse(script, {'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT': value},
                   3 if script == 'build_qwen38_inference.sh' else 2)
    for value in ['2', '-1', 'invalid']:
        refuse('build_qwen38_ffn_graph_image.sh',
               {'PIONEER_Q38_FFN_GRAPH_ROOT': '1', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT': value})
    refuse('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT': '1'})
    for key in ['PIONEER_Q38_FFN_RESIDENT_MATRIX', 'PIONEER_Q38_FFN_PERSISTENT',
                'PIONEER_Q38_OUTPUT_TRANSFER_ROOT', 'PIONEER_Q38_WORK_ROOT_COMMAND_REUSE',
                'PIONEER_Q38_WORK_BATCH4_FETCH', 'PIONEER_Q38_PEER_PHASE_ROOT',
                'PIONEER_Q38_WORK_RAW_UNPACK_CACHE', 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION',
                'PIONEER_Q38_WORK_LOCAL_Q4', 'PIONEER_Q38_LOCAL_Q4_QUALIFICATION']:
        for value in ['1', '-1', 'invalid']:
            refuse('build_qwen38_ffn_graph_image.sh',
                   {'PIONEER_Q38_FFN_GRAPH_ROOT': '1', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT': '1', key: value})
    # Positive role reaches the deliberately absent SDK;the test can
    # distinguish a real early refusal from a later failed toolchain lookup.
    probe = subprocess.run(['sh', str(root / 'build_qwen38_ffn_graph_image.sh'), sdk, str(output)],
                           env=dict(environment, PIONEER_Q38_FFN_GRAPH_ROOT='1',
                                    PIONEER_Q38_FFN_GRAPH_INPUT_ROOT='1'), text=True, capture_output=True)
    assert probe.returncode and sdk in probe.stderr and not output.exists()
print(f'PASS {count} QFG2 mixed/malformed/legacy/native/production role refusals BEFORE SDK/output;valid role reaches missing-SDK control;NO hardware')

if len(sys.argv) == 3:
    # The existing frozen QFG1 catalog must not silently accept QFG2 bytes.
    result = subprocess.run([sys.executable, str(root / 'generate_qwen38_ffn_graph_images.py'),
                             sys.argv[1], sys.argv[2], 'root-main'], text=True, capture_output=True)
    assert result.returncode and not result.stdout and 'pinned private section mismatch' in result.stderr
    print('PASS old pinned QFG1 frozen catalog refuses QFG2 before ANY patch output;NO frozen/native replacement')
else:
    assert len(sys.argv) == 1
