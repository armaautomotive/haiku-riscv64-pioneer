#!/usr/bin/env python3
"""Exercise the matched-band verifier against real private fixtures and tampering."""
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 5:
    raise SystemExit('use BFP4_BAND Q4_INPUT Q4_EXPECTED Q4_PHYSICAL_LOG')
verifier = Path(__file__).with_name('compare_pioneer_q38_matched_band.py')
paths = [Path(arg) for arg in sys.argv[1:]]
def check(arguments, okay):
    result = subprocess.run([sys.executable, str(verifier), *map(str, arguments)], capture_output=True, text=True)
    if (result.returncode == 0) != okay:
        raise RuntimeError('unexpected verifier status')
    if okay and 'Q4_CARD_WORDS64_EXACT BFP4_CPU_PREDICTION32 input_bytes10240' not in result.stdout:
        raise RuntimeError('missing matched-output scope')

check(paths, True)
with tempfile.TemporaryDirectory(prefix='pioneer-matched-band-test-') as directory:
    for i, path in enumerate(paths):
        changed = bytearray(path.read_bytes())
        if not changed:
            raise ValueError('empty fixture')
        changed[len(changed)//2] ^= 1
        bad = Path(directory) / str(i)
        bad.write_bytes(changed)
        arguments = list(paths)
        arguments[i] = bad
        check(arguments, False)
print('PASS matched-band comparison and four changed-fixture refusals; NO NEW CARD RUN')
