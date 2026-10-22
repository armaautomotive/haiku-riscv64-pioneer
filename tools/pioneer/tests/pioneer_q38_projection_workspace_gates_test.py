#!/usr/bin/env python3
"""Private workspace role isolation/EXACT old zero proof reuse. NO device."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
root = directory.parent.parent
if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen zero image')
assert hashlib.sha256((pathlib.Path(sys.argv[1]) / 'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
old = (root / 'src/add-ons/kernel/drivers/misc/PioneerQ38FfnZeroController.h').read_text()
new = (root / 'src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionZeroController.h').read_text()
expected = old[old.index('struct PioneerQ38FfnZeroLease'):].replace('PioneerQ38Ffn', 'PioneerQ38Projection')
expected = expected.replace('StateZeroBeforeCohort', 'ProjectionZeroBeforeCohort').replace('StateZeroOtherCoresHeld', 'ProjectionZeroOtherCoresHeld')
expected = expected.replace('if (region || !pages || firstPage>=PioneerQ38ProjectionBanks::Pages',
                            'if (region || !pages || pages>TT_Q38_ZERO_BYTES/4096 || firstPage>=PioneerQ38ProjectionBanks::Pages')
assert new[new.index('struct PioneerQ38ProjectionZeroLease'):] == expected, 'new controller must preserve ALL frozen ROM source/code/PC/counter/summary/terminal/restore/publication proofs'
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-workspace-gates-') as temporary:
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
            result = subprocess.run(args, env={**clean, **owner, 'PIONEER_Q38_PROJECTION_WORKSPACE': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, value, result)
            cases += 1
print(f'PASS {cases} before-SDK projection workspace refusals;EXACT frozen zero ROM pin and ALL old controller source/code/counters/PC/restore/publication proofs retained with DISTINCT new callbacks+512page state+1MiB/run limit;NO_NATIVE_SELECTOR_OR_INPUT_PRODUCER_GRANT')
