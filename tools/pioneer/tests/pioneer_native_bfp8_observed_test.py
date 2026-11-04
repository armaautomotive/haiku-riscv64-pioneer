#!/usr/bin/env python3
"""Regression over immutable ACTUAL returned data;never opens a device."""
import contextlib
import hashlib
import io
import pathlib
import re
import runpy
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
record = here.parent / 'benchmarks/qwen38-native-bfp8-observe-20261104/fixed8-returned.stderr'
raw = record.read_bytes()
assert len(raw) == 4691 and hashlib.sha256(raw).hexdigest() == '66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36'
with contextlib.redirect_stdout(io.StringIO()):
    reference = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
compare = runpy.run_path(str(here / 'compare_pioneer_native_bfp_arithmetic_words.py'))['compare']
assert compare(raw.decode(), 8) == 64
words = {}
different = 0
for line in raw.decode().splitlines():
    match = re.fullmatch(r'NATIVE_BFP_RETURNED_WORD bits8 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
    if not match:
        assert line == 'NATIVE_BFP_NUMERICAL_MISMATCH bits8 pass0 row0 got4180b500 expected4180b340;CLOSE_ONLY_NO_END_OR_RETRY'
        continue
    job, row = map(int, match.group(1, 2))
    got, old_expected = (int(x, 16) for x in match.group(3, 4))
    ideal, predicted = reference['results'][8][row]
    assert got == predicted and old_expected == ideal and (job, row) not in words
    words[(job, row)] = got
    different += got != ideal
assert len(words) == 64 and different == 60 and all(words[(0, row)] == words[(1, row)] for row in range(32))
print('PASS ACTUAL recorded BFP8 data:64/64 exact documented-arithmetic predictions,dual jobs identical,60/64 differ from strict ideal;source record SHA/length pinned,old trial remains REJECTED/NO END,NO new device execution/full-K/model-quality/throughput claim')
