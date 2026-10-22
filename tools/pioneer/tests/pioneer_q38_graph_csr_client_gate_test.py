#!/usr/bin/env python3
"""Fixed tag31 debug role refuses BEFORE SDK/output;no card operations."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
fixed = dict(PIONEER_Q38_GRAPH_CSR_REFERENCE_CLIENT='1', PIONEER_Q38_SENTINEL_REFERENCE_CLIENT='1',
             PIONEER_Q38_PANEL_CLIENT='4', PIONEER_Q38_VECTOR_CLIENT='1',
             PIONEER_Q38_WORK_CLIENT='1', PIONEER_Q38_ROOT_CYCLE_CLIENT='1')
forbidden = [
    'LOCAL_Q4', 'Q4_DIRECT', 'Q4_NIBBLE', 'PEER_PHASE', 'COMMAND', 'PREFETCH', 'RAW_CACHE',
    'FRONTEND', 'BATCH_RAW', 'BATCH4', 'HOST_PHASE', 'NATIVE_Q8', 'GROUP32', 'AFFINE_RAWDOT', 'DIRECT_SRCB',
]
forbidden = ['PIONEER_Q38_' + name + '_REFERENCE_CLIENT' for name in forbidden] + [
    'PIONEER_Q38_REUSE_CLIENT', 'PIONEER_Q38_AFFINE_CLIENT', 'PIONEER_Q38_FIXTURE_CLIENT',
    'PIONEER_Q38_BLOCK_CLIENT', 'PIONEER_Q38_LOCAL_DELTA_GGML', 'PIONEER_Q38_COHORT_SEALED',
    'PIONEER_Q38_COHORT_PAYLOAD', 'PIONEER_Q38_ROWS_CLIENT', 'PIONEER_Q38_STREAM_DIAGNOSTICS',
    'PIONEER_Q38_PRIMITIVE_TIMING',
]
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-csr-client-gates-') as name:
    base = pathlib.Path(name)

    def refuse(changes, builder='build_qwen38_banks_client.sh', defaults=fixed):
        global cases
        environment = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
        environment.update(defaults)
        for key, value in changes.items():
            if value is None:
                environment.pop(key, None)
            else:
                environment[key] = value
        sdk = base / 'MUST_NOT_OPEN_SDK'
        output = base / ('forbidden-' + str(cases))
        args = ['sh', str(directory / builder), str(sdk), str(output)]
        if builder == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env=environment, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists(), (changes, result.stdout, result.stderr)
        assert str(sdk) not in result.stdout + result.stderr, (changes, 'SDK_OPENED')
        cases += 1

    # Missing/zero new flag deliberately retains the OLD tag17 client;do not
    # accidentally prohibit that previously qualified build role.
    for key in fixed:
        for bad in ([None, '0', '01', '-1', 'bad'] if key != 'PIONEER_Q38_GRAPH_CSR_REFERENCE_CLIENT' else ['01', '-1', '2', 'bad']):
            refuse({key: bad})
    for key in forbidden:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({key: bad})
    for key in ['PIONEER_Q38_GRAPH_CSR_REFERENCE_CLIENT', 'PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC']:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({key: bad}, 'build_qwen38_inference.sh', {})
print(f'PASS {cases} fixed tag31 debug-client/mixed-role/production-inference refusals BEFORE SDK/output;NO_NATIVE_ACCESS')
