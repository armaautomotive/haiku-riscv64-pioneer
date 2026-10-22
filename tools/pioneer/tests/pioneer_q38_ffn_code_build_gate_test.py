#!/usr/bin/env python3
"""Complete held-only FFN code diagnostic rejects BEFORE SDK/output."""
import os
import pathlib
import re
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
fixed = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)\b', (directory / 'build_qwen38_ffn_code_diagnostic.sh').read_text()))
assert len(fixed) >= 35
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-code-gates-') as name:
    base = pathlib.Path(name)

    def refuse(changes, script='build_qwen38_banks_driver.sh', defaults=fixed):
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
        for bad in [None, '0', '01', '-1', 'bad']:
            refuse({key: bad})
    for key in ['PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC', 'PIONEER_Q38_LOCAL_Q4_QUALIFICATION',
                'PIONEER_Q38_Q4_NIBBLE_QUALIFICATION', 'PIONEER_Q38_Q4_DIRECT_QUALIFICATION',
                'PIONEER_Q38_SIGNED_MAG2_QUALIFICATION', 'PIONEER_Q38_PEER_PHASE_QUALIFICATION',
                'PIONEER_Q38_COMMAND_REUSE_QUALIFICATION', 'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION',
                'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION', 'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION',
                'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION', 'PIONEER_Q38_HOST_PHASE_PROFILE',
                'PIONEER_Q38_GDN_MUTABLE_STATE', 'PIONEER_Q38_RESIDENT_GDN', 'PIONEER_Q38_LOCAL_DELTA_GGML',
                'PIONEER_Q38_DISPATCH_PROFILE', 'PIONEER_Q38_FFN_GRAPH_ROOT', 'PIONEER_Q38_FFN_PERSISTENT',
                'PIONEER_Q38_FFN_RESIDENT_MATRIX']:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({key: bad})
    for bad in ['1', '01', '-1', 'bad']:
        refuse({'PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT': bad})
    for script in ['build_qwen38_inference.sh', 'build_qwen38_banks_client.sh']:
        for bad in ['1', '01', '-1', 'bad']:
            refuse({'PIONEER_Q38_FFN_CODE_DIAGNOSTIC': bad}, script, {})
            refuse({'PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT': bad}, script, {})
print(f'PASS {cases} held-only FFN code tag32/missing/malformed/mixed-role/production refusals BEFORE SDK/output;NO_NATIVE_ACCESS')
