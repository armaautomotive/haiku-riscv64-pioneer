#!/usr/bin/env python3
"""Exercise host-only preparer isolation without reading any model or device."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

if sys.platform != 'darwin':
    raise SystemExit('macOS-only test')
root = Path(__file__).resolve().parents[3]
builder = root / 'tools/pioneer/build_q38_host_preparers.py'
with tempfile.TemporaryDirectory(prefix='q38-host-test-') as temp:
    base = Path(temp)
    builds = [base / 'first', base / 'second']
    for build in builds:
        subprocess.run([sys.executable, str(builder), str(build)], check=True)
    manifests = [json.loads((build / 'manifest.json').read_text()) for build in builds]
    assert set(manifests[0]) == {'bfp4', 'head-bfp8'}
    for name in manifests[0]:
        assert manifests[0][name]['fingerprint'] == manifests[1][name]['fingerprint']
        assert manifests[0][name]['fingerprint'] not in {
            '5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c',
            'e0e45dce6f1d95124aaeb956e2ee4076aff5b11d35d8079e205054cf33cf316f'}
        mode = '--prepare-qgw8-bfp4' if name == 'bfp4' else '--prepare-head-bfp8'
        binary = builds[0] / ('pioneer-q38-host-' + name + '-prepare')
        cache = base / ('cache-' + name)
        for args, expected in [([], 2), ([mode, str(cache), '--model', 'relative'], 2),
                               ([mode, str(cache), '--model', str(base / 'missing')], 1)]:
            result = subprocess.run([str(binary), *args], capture_output=True)
            assert result.returncode == expected, (name, args, result.returncode, result.stderr)
            assert not cache.exists()
    sources = [('pioneer_q38_hybrid_bfp4_prepare.cpp', 'PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC',
                'PIONEER_BFP4_IMPLEMENTATION_SHA256'),
               ('pioneer_q38_head_bfp8_prepare.cpp', 'PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC',
                'PIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256')]
    for source, role, fingerprint in sources:
        for forbidden in ['_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_WORK_INFERENCE',
                          'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                          'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_CPU_VALIDATION',
                          'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE']:
            command = ['c++', '-std=c++17', '-E', '-DPIONEER_Q38_HOST_PREPARATION=1',
                       '-D' + role + '=1', '-D' + fingerprint + '=\"' + '0' * 64 + '\"',
                       '-D' + forbidden + '=1', str(root / 'tools/pioneer' / source)]
            result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            assert result.returncode != 0 and b'isolated macOS preparation only' in result.stderr, (source, forbidden)
print('PASS: host preparation keys, CLI isolation and forbidden roles')
