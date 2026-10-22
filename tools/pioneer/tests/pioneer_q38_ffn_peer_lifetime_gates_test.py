#!/usr/bin/env python3
"""Exact tag33/NS6 driver+debug-client BEFORE-SDK refusal matrix."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent.parent
driver = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)\b', (here / 'build_qwen38_ffn_peer_lifetime_diagnostic.sh').read_text()))
assert len(driver) >= 40 and driver['PIONEER_DRIVER_BUILD_TAG'] == '2026102200010033'
client = dict(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT='1', PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT='1',
              PIONEER_Q38_SENTINEL_REFERENCE_CLIENT='1', PIONEER_Q38_PANEL_CLIENT='4',
              PIONEER_Q38_VECTOR_CLIENT='1', PIONEER_Q38_WORK_CLIENT='1', PIONEER_Q38_ROOT_CYCLE_CLIENT='1')
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-peer-lifetime-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(defaults, changes, script):
        global count
        env = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
        env.update(defaults)
        for key, value in changes.items():
            if value is None:
                env.pop(key, None)
            else:
                env[key] = value
        sdk, out = base / 'MUST_NOT_OPEN_SDK', base / ('forbidden-' + str(count))
        args = ['sh', str(here / script), str(sdk), str(out)]
        if script == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        r = subprocess.run(args, env=env, capture_output=True, text=True, timeout=5)
        assert r.returncode == 2 and not out.exists() and str(sdk) not in r.stdout + r.stderr, (changes, r.returncode, r.stdout, r.stderr)
        count += 1

    for key in driver:
        for value in [None, '0', '01', '-1', 'bad']:
            refuse(driver, {key: value}, 'build_qwen38_banks_driver.sh')
    for key in ['PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT', 'PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT',
                'PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC', 'PIONEER_Q38_LOCAL_Q4_QUALIFICATION',
                'PIONEER_Q38_Q4_NIBBLE_QUALIFICATION', 'PIONEER_Q38_Q4_DIRECT_QUALIFICATION',
                'PIONEER_Q38_SIGNED_MAG2_QUALIFICATION', 'PIONEER_Q38_PEER_PHASE_QUALIFICATION',
                'PIONEER_Q38_COMMAND_REUSE_QUALIFICATION', 'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION',
                'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION', 'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION',
                'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION', 'PIONEER_Q38_HOST_PHASE_PROFILE',
                'PIONEER_Q38_GDN_MUTABLE_STATE', 'PIONEER_Q38_RESIDENT_GDN', 'PIONEER_Q38_LOCAL_DELTA_GGML',
                'PIONEER_Q38_DISPATCH_PROFILE', 'PIONEER_Q38_FFN_GRAPH_ROOT', 'PIONEER_Q38_FFN_PERSISTENT',
                'PIONEER_Q38_FFN_RESIDENT_MATRIX']:
        for value in ['1', '01', '-1', 'bad']:
            refuse(driver, {key: value}, 'build_qwen38_banks_driver.sh')
    for value in ['2026102200010032', '2026102200010031', '2026102100010012']:
        refuse(driver, {'PIONEER_DRIVER_BUILD_TAG': value}, 'build_qwen38_banks_driver.sh')
    for key in client:
        # Explicitly omitting the new role selects the legitimate OLD code
        # client;the new wrapper always sets it. Malformed roles refuse.
        values = ['01', '-1', '2', 'bad'] if key.endswith('LIFETIME_REFERENCE_CLIENT') else [None, '0', '01', '-1', 'bad']
        for value in values:
            refuse(client, {key: value}, 'build_qwen38_banks_client.sh')
    for name in ['GRAPH_CSR', 'LOCAL_Q4', 'Q4_DIRECT', 'Q4_NIBBLE', 'PEER_PHASE', 'COMMAND', 'PREFETCH',
                 'RAW_CACHE', 'FRONTEND', 'BATCH_RAW', 'BATCH4', 'HOST_PHASE', 'NATIVE_Q8', 'GROUP32', 'AFFINE_RAWDOT', 'DIRECT_SRCB']:
        for value in ['1', '01', '-1', 'bad']:
            refuse(client, {'PIONEER_Q38_' + name + '_REFERENCE_CLIENT': value}, 'build_qwen38_banks_client.sh')
    for key in ['PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT']:
        for value in ['1', '01', '-1', 'bad']:
            refuse({}, {key: value}, 'build_qwen38_inference.sh')
    refuse(client, {'PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC': '1'}, 'build_qwen38_banks_client.sh')
print(f'PASS {count} exact tag33/NS6 driver+client/missing/malformed/mixed/production refusals BEFORE SDK/output;NO_NATIVE_ACCESS')
