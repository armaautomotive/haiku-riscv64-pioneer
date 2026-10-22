#!/usr/bin/env python3
"""Reject bad fixed diagnostic roles BEFORE SDK/output creation."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent.parent
builder = here / 'build_qwen38_banks_driver.sh'
wrapper = here / 'build_qwen38_graph_csr_diagnostic.sh'
fixed = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)\b', wrapper.read_text()))
assert len(fixed) >= 35
refusals = 0
with tempfile.TemporaryDirectory(prefix='pioneer-graph-csr-build-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(changes):
        global refusals
        environment = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
        environment.update(fixed)
        for key, value in changes.items():
            if value is None:
                environment.pop(key, None)
            else:
                environment[key] = value
        output = base / ('forbidden-' + str(refusals))
        sdk = base / 'MUST_NOT_OPEN_SDK'
        result = subprocess.run(['sh', str(builder), str(sdk), str(output)], env=environment, capture_output=True, text=True, check=False)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr
        refusals += 1

    for key, value in fixed.items():
        for bad in [None, '0', '01', '-1', 'invalid']:
            refuse({key: bad})
    forbidden = [
        'PIONEER_Q38_LOCAL_Q4_QUALIFICATION', 'PIONEER_Q38_Q4_NIBBLE_QUALIFICATION', 'PIONEER_Q38_Q4_DIRECT_QUALIFICATION',
        'PIONEER_Q38_SIGNED_MAG2_QUALIFICATION', 'PIONEER_Q38_PEER_PHASE_QUALIFICATION', 'PIONEER_Q38_COMMAND_REUSE_QUALIFICATION',
        'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION', 'PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION', 'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION',
        'PIONEER_Q38_BATCH4_FETCH_QUALIFICATION', 'PIONEER_Q38_HOST_PHASE_PROFILE', 'PIONEER_Q38_GDN_MUTABLE_STATE',
        'PIONEER_Q38_RESIDENT_GDN', 'PIONEER_Q38_LOCAL_DELTA_GGML', 'PIONEER_Q38_DISPATCH_PROFILE',
        'PIONEER_Q38_FFN_GRAPH_ROOT', 'PIONEER_Q38_FFN_PERSISTENT', 'PIONEER_Q38_FFN_RESIDENT_MATRIX',
    ]
    for key in forbidden:
        for bad in ['1', '01', '-1', 'invalid']:
            refuse({key: bad})
print(f'PASS fixed graph-CSR driver tag31:{refusals} missing/malformed/reserved/mixed-role refusals BEFORE SDK/output;NO_NATIVE_ACCESS')
