#!/usr/bin/env python3
"""Real early builder/header/native refusal gates;NO SDK or device access."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
headers = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-decoder-runtime-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image',
                 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image',
                 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_DECODER_RUNTIME' in line]
        assert len(guards) == 1
        guard = 'set -eu\n' + guards[0] + '\nprintf GUARD_ACCEPT\n'
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_DECODER_RUNTIME': value}
            result = subprocess.run(['sh', '-c', guard], env=env, capture_output=True, text=True, check=True)
            assert result.stdout == 'GUARD_ACCEPT'
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            for command in (['sh', '-c', guard], ['sh', str(script), str(sdk), str(output)]):
                result = subprocess.run(command, env={**clean, 'PIONEER_Q38_DECODER_RUNTIME': value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
    # All independent new image/native Python builders already reject every
    # unrecognized nonzero Pioneer flag. Prove the actual commands refuse.
    for name in ('build_qwen38_conv_image.sh', 'build_qwen38_conv_batch_image.sh',
                 'build_qwen38_projection_validation_native.py'):
        builder = directory / name
        command = ['python3' if name.endswith('.py') else 'sh', str(builder), str(sdk), str(output)]
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(command, env={**clean, 'PIONEER_Q38_DECODER_RUNTIME': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    # The ACTUAL driver fails this private role unconditionally,including
    # explicitly defined0: it cannot silently inherit old tag42/Work/native.
    source = (headers / 'tenstorrent_pci_probe.cpp').read_text()
    guard = re.findall(r'#ifdef PIONEER_Q38_DECODER_RUNTIME\n#error [^\n]+\n#endif', source)
    assert len(guard) == 1
    for value in ('0', '1', '2', '-1'):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', '-DPIONEER_Q38_DECODER_RUNTIME=' + value, '-'],
                                input=guard[0], env=clean, capture_output=True, text=True)
        assert result.returncode != 0 and 'Decoder runtime/history reservation is OFFLINE' in result.stderr
        count += 1
    for header in ('PioneerQ38ProjectionMutableBanks.h', 'PioneerQ38ConvHistoryBanks.h', 'PioneerQ38ConvHistoryZeroController.h'):
        for value in (None, '0', '1', '2', '-1'):
            args = [os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-fsyntax-only', '-DPIONEER_Q38_GDN_MUTABLE_STATE']
            if value is not None:
                args.append('-DPIONEER_Q38_DECODER_RUNTIME=' + value)
            result = subprocess.run([*args, '-'], input='#include "' + str(headers / header) + '"\n', env=clean, capture_output=True, text=True)
            assert (result.returncode == 0) == (value == '1' or value is None and header == 'PioneerQ38ProjectionMutableBanks.h')
            count += 1
print(f'PASS decoder runtime gates{count},actual old/new builders refuse BEFORE SDK/output,exact private header role,actual driver HARD REFUSAL;NO native selector/Work promotion/install/device access')
