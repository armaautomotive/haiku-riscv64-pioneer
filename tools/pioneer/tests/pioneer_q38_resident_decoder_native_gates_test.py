#!/usr/bin/env python3
"""Actual tag43/native role/compiler/builder guards;NO SDK/device execution."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0]
assert prefix.count('#include "tenstorrent_pci_probe.h"') == 1
prefix = prefix.replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
valid = dict(PIONEER_Q38_RESIDENT_DECODER_NATIVE='1', PIONEER_Q38_RESIDENT_DECODER_OWNER='1', PIONEER_Q38_DECODER_RUNTIME='1',
             PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1', PIONEER_Q38_PROJECTION_RESULT_STASH='1', PIONEER_Q38_LEARNED_RMS_NATIVE='1',
             PIONEER_Q38_EMBEDDING_NATIVE='1', TT_PIONEER_DRIVER_RELOAD='1', PIONEER_DRIVER_BUILD_TAG='0x2026102300010043ULL')
count = 0
def preprocess(flags, accepted):
    global count
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                            input=prefix, env=clean, capture_output=True, text=True)
    assert (result.returncode == 0) == accepted, (flags, result.stderr)
    count += 1
preprocess({}, True)
preprocess(valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        bad = dict(valid)
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        preprocess(bad, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010039ULL', '0x2026102200010040ULL', '0x2026102200010041ULL', '0x2026102200010042ULL'):
    preprocess({**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess({'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE',
             'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
             'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess({**valid, flag: value}, False)
for flag in ('PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME'):
    for value in ('0', '1', '2', '-1'):
        preprocess({flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-decoder-native-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_RESIDENT_DECODER_NATIVE' in line]
        assert len(guards) == 1
        guard = 'set -eu\n' + guards[0] + '\nprintf GUARD_ACCEPT\n'
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_RESIDENT_DECODER_NATIVE': value}
            result = subprocess.run(['sh', '-c', guard], env=env, capture_output=True, text=True, check=True)
            assert result.stdout == 'GUARD_ACCEPT'
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            for command in (['sh', '-c', guard], ['sh', str(script), str(sdk), str(output)]):
                result = subprocess.run(command, env={**clean, 'PIONEER_Q38_RESIDENT_DECODER_NATIVE': value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
    for script in ('build_qwen38_projection_validation_native.py', 'tests/pioneer_q38_resident_decoder_native_full_compile.py'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(['python3', str(directory / script), str(sdk), str(output)],
                                    env={**clean, 'PIONEER_Q38_RESIDENT_DECODER_NATIVE': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
assert 'PIONEER_Q38_RESIDENT_DECODER_NATIVE_CONTROL_BEGIN' not in source, 'new selector must be separately qualified'
print(f'PASS actual native tag43 gates{count},only complete exact role accepted/reserved tag cannot alias/old tags and DEBUG-Work mixes denied/actual legacy builders refuse BEFORE SDK/output/full compile refuses environment promotion;NO new public selector/install/device access')
