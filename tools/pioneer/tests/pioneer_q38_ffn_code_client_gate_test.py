#!/usr/bin/env python3
"""Exact tag32/NS5 debug role refuses BEFORE SDK/output;NO native access."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
fixed = dict(PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT='1', PIONEER_Q38_SENTINEL_REFERENCE_CLIENT='1',
             PIONEER_Q38_PANEL_CLIENT='4', PIONEER_Q38_VECTOR_CLIENT='1', PIONEER_Q38_WORK_CLIENT='1',
             PIONEER_Q38_ROOT_CYCLE_CLIENT='1')
forbidden = ['GRAPH_CSR', 'LOCAL_Q4', 'Q4_DIRECT', 'Q4_NIBBLE', 'PEER_PHASE', 'COMMAND', 'PREFETCH',
             'RAW_CACHE', 'FRONTEND', 'BATCH_RAW', 'BATCH4', 'HOST_PHASE', 'NATIVE_Q8', 'GROUP32',
             'AFFINE_RAWDOT', 'DIRECT_SRCB']
forbidden = ['PIONEER_Q38_' + name + '_REFERENCE_CLIENT' for name in forbidden] + [
    'PIONEER_Q38_REUSE_CLIENT', 'PIONEER_Q38_AFFINE_CLIENT', 'PIONEER_Q38_FIXTURE_CLIENT',
    'PIONEER_Q38_BLOCK_CLIENT', 'PIONEER_Q38_LOCAL_DELTA_GGML', 'PIONEER_Q38_COHORT_SEALED',
    'PIONEER_Q38_COHORT_PAYLOAD', 'PIONEER_Q38_ROWS_CLIENT', 'PIONEER_Q38_STREAM_DIAGNOSTICS',
    'PIONEER_Q38_PRIMITIVE_TIMING', 'PIONEER_Q38_FFN_CODE_DIAGNOSTIC',
]
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-code-client-gates-') as name:
    base = pathlib.Path(name)

    def refuse(changes, script='build_qwen38_banks_client.sh', defaults=fixed):
        global cases
        env = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
        env.update(defaults)
        for key, value in changes.items():
            if value is None:
                env.pop(key, None)
            else:
                env[key] = value
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('forbidden-' + str(cases))
        args = ['sh', str(directory / script), str(sdk), str(output)]
        if script == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        r = subprocess.run(args, env=env, capture_output=True, text=True, timeout=5)
        assert r.returncode == 2 and not output.exists() and str(sdk) not in r.stdout + r.stderr, (changes, r.stdout, r.stderr)
        cases += 1

    for key in fixed:
        bad_values = ['01', '-1', '2', 'bad'] if key == 'PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT' else [None, '0', '01', '-1', 'bad']
        for bad in bad_values:
            refuse({key: bad})
    for key in forbidden:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({key: bad})
    for key in ['PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT', 'PIONEER_Q38_FFN_CODE_DIAGNOSTIC']:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({key: bad}, 'build_qwen38_inference.sh', {})
print(f'PASS {cases} exact tag32/NS5 client/mixed-role/production refusals BEFORE SDK/output;NO_NATIVE_ACCESS')
