#!/usr/bin/env python3
"""QPS1 is explicit offline data/owner opt-in,never old/native promotion."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parents[1]
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-stash-gates-') as temporary:
    base = pathlib.Path(temporary)
    sdk, output = base / 'MUST_NOT_READ_SDK', base / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image',
                 'learned_rms_image', 'embedding_image', 'projection_input_image', 'projection_gather_image'):
        for value in ('1', '2', '01', '', '-1', '0 ', '00', '0\n1'):
            result = subprocess.run(['sh', str(here / ('build_qwen38_' + name + '.sh')), str(sdk), str(output)],
                                    env={**clean, 'PIONEER_Q38_PROJECTION_RESULT_STASH': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, value, result)
            count += 1
    for args in ([], [str(sdk)], [str(sdk), str(output), '--stash'],
                 [str(sdk), str(output), '--result-stashes', 'extra']):
        result = subprocess.run(['sh', str(here / 'build_qwen38_projection_graph_image.sh'), *args],
                                env=clean, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
        count += 1
source = (here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
start = '#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH\n'
guard = start + source.split(start, 1)[1].split('#endif\n#endif\n', 1)[0] + '#endif\n#endif\n'
valid = ['-DPIONEER_Q38_PROJECTION_RESULT_STASH=1', '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1',
         '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_NATIVE=1']
invalid = []
for index in range(4):
    for value in ('0', '2', '-1'):
        invalid.append(valid[:index] + [valid[index].split('=')[0] + '=' + value] + valid[index+1:])
for index in range(1, 4):
    invalid.append(valid[:index] + valid[index+1:])
invalid += [valid + ['-D' + role + '=1'] for role in (
    'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE',
    'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE')]
for flags in invalid:
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *flags, '-'], input=guard,
                            capture_output=True, text=True, env=clean)
    assert result.returncode != 0 and 'Fixed QPS1 result stashes' in result.stderr
    count += 1
for flags in ([], valid):
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *flags, '-'], input=guard,
                            capture_output=True, text=True, env=clean)
    assert result.returncode == 0
    count += 1
print(f'PASS QPS1 stash isolation gates{count}:old builders refuse before SDK/output;wrong/missing owner and debug/public/CPU roles rejected;NO native selector/install')
