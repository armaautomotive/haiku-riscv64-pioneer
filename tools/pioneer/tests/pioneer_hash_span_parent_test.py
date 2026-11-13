#!/usr/bin/env python3
"""Offline tests of the actual bounded-parent output and failure gates."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'pioneer_hash_span_trial_20261112.sh').read_text()
subprocess.run(['sh', '-n', str(root / 'pioneer_hash_span_trial_20261112.sh')], check=True)
start = source.index('check_output(){')
end = source.index('\nrun_trial(){', start)
function = source[start:end]
header = 'PUBLIC_SPAN_TRANSPORT_ONLY;NO_MODEL_OR_INFERENCE;FAILURE_CLOSE_ONLY_NO_END_RETRY_RESET\n'
jobs = ''.join(f'PUBLIC_SPAN bank{bank} bytes4096 pages1 nonce{n+1} seconds0.001000 known_held1 cpu_transport_checked0;NOT_MODEL_OR_TOKEN_RATE\n'
               for n, bank in enumerate([0, 1, 2, 3, 4, 5, 6] * 3))
readback = 'PUBLIC_WORK_SPAN_DEBUG_READBACK matched_bytes7512064;EVERY_ORIGINAL_BYTE_COMPARED_DEBUG_CLIENT_ONLY\n'
passed = 'PASS_PUBLIC_SPAN_TRANSPORT jobs21 bytes7512064 upload_seconds1.000000 host_MB_per_second7.512064 clean_end1;NOT_PURE_PCIE_BANDWIDTH_OR_LLM_THROUGHPUT\n'
with tempfile.TemporaryDirectory(prefix='pioneer-hash-span-parent-') as tmp:
    p = Path(tmp) / 'case'
    (Path(str(p) + '.exit')).write_text('0\n')
    (Path(str(p) + '.stderr')).write_text('')
    for body, expected in ((header+jobs+readback+passed, True),
                           (header+jobs+passed, False),
                           (header+jobs+readback+passed.replace('clean_end1', 'clean_end0'), False),
                           (header+jobs+readback+passed+'extra\n', False),
                           (header+jobs+readback+passed.replace('jobs21', 'jobs20'), False)):
        (Path(str(p) + '.stdout')).write_text(body)
        result = subprocess.run(['sh', '-c', function+'\ncheck_output "$1"', 'sh', str(p)])
        assert (result.returncode == 0) == expected
    (Path(str(p) + '.stdout')).write_text(header+jobs+readback+passed)
    (Path(str(p) + '.stderr')).write_text('error\n')
    assert subprocess.run(['sh', '-c', function+'\ncheck_output "$1"', 'sh', str(p)]).returncode
    (Path(str(p) + '.stderr')).write_text('')
    (Path(str(p) + '.exit')).write_text('1\n')
    assert subprocess.run(['sh', '-c', function+'\ncheck_output "$1"', 'sh', str(p)]).returncode

# The irreversible driver exchanges are gated on the immediately preceding
# known-clean trial. No trap or background retry may run after an unknown.
assert 'trap ' not in source
assert 'run_trial baseline 2026102100010012 40 "$stable_hash" || exit 70\nexchange ' in source
assert 'run_trial candidate-run 2026111200010002 41 "$candidate_hash" || exit 72\nexchange ' in source
assert 'UNPROVEN_${name}_NO_RETRY_RESET_OR_RESTORE' in source
assert 'CLOSED_STABLE_RESTORED' in source
print('PASS bounded parent output gates and no-restore-on-unknown structure; OFFLINE ONLY')
