#!/usr/bin/env python3
"""Private execution role cannot be selected through any old builder."""
import os
import pathlib
import subprocess
import tempfile
here = pathlib.Path(__file__).resolve().parents[1]
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-execution-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image', 'learned_rms_image'):
        for value in ('1', '2', '01', '', '-1', '0 ', '00', '0\n1'):
            result = subprocess.run(['sh', str(here / ('build_qwen38_' + name + '.sh')), str(sdk), str(output)],
                                    env={**clean, 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER': value}, capture_output=True, text=True)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
            count += 1
    source = (here.parents[1] / 'src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphOwner.h').read_text()
    guard = '#if ' + source.split('#if ', 1)[1].split('#endif', 1)[0] + '#endif\n'
    for flags in ([], ['-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=0'], ['-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=2'],
                  ['-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1'],
                  ['-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=0'],
                  ['-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=2']):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *flags, '-'], input=guard,
                                capture_output=True, text=True, env=clean)
        assert result.returncode != 0 and 'Private complete projection execution owner' in result.stderr
        count += 1
print(f'PASS projection execution gates{count};all old builders refuse BEFORE SDK/output;actual private role guard rejects missing/wrong producer;NO native selector/install')
