#!/usr/bin/env python3
"""Only SYNTHETIC returned-word records;never represents a physical run."""
import contextlib
import io
import pathlib
import runpy
import subprocess
import sys
import tempfile

if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    arithmetic = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
compare = runpy.run_path(str(here / 'compare_pioneer_native_bfp_arithmetic_words.py'))['compare']
cases = 0
for bits in (4, 8):
    lines = [f'NATIVE_BFP_RETURNED_WORD bits{bits} pass{job} row{row} got{arithmetic["results"][bits][row][1]:08x} expected{arithmetic["results"][bits][row][0]:08x}' for job in range(2) for row in range(32)]
    assert compare('\n'.join(lines), bits) == 64
    cases += 1
    bad_records = [lines[:-1], lines + lines[:1], ['noise only'], [line.replace(f'bits{bits}', f'bits{12-bits}') for line in lines],
                   ['NATIVE_BFP_RETURNED_WORD malformed'] + lines,
                   [lines[0].replace('row0 ', 'row32 ')] + lines[1:],
                   [lines[0].split(' expected')[0] + ' expected00000000'] + lines[1:]]
    for index in range(64):
        bad = list(lines);bad[index] = bad[index].replace(' got', ' got0', 1);bad_records.append(bad)
        bad = list(lines);pieces = bad[index].split(' got');word, tail = pieces[1].split(' expected');bad[index] = pieces[0] + f' got{int(word,16)^1:08x} expected' + tail;bad_records.append(bad)
    for bad in bad_records:
        try:
            compare('\n'.join(bad), bits)
        except ValueError:
            cases += 1
        else:
            raise AssertionError('Bad/incomplete synthetic record accepted')
old = (here.parent / 'benchmarks/qwen38-native-bfp-physical-20261103/physical.log').read_text()
try:
    compare(old, 8)
except ValueError:
    cases += 1
else:
    raise AssertionError('Old single observed mismatch was promoted to64-word proof')
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-arithmetic-records.'))
record = temporary / 'SYNTHETIC.txt'
record.write_text('\n'.join(lines) + '\n')
program = here / 'compare_pioneer_native_bfp_arithmetic_words.py'
valid = subprocess.run([sys.executable, str(program), str(record), '--bits', '8'], text=True, capture_output=True)
assert valid.returncode == 0 and 'RECORD_PROVENANCE_AND_PHYSICAL_RUN_NOT_AUTHENTICATED' in valid.stdout
cases += 1
record.write_bytes(b'X' * (1024 * 1024 + 1))
oversized = subprocess.run([sys.executable, str(program), str(record), '--bits', '8'], text=True, capture_output=True)
assert oversized.returncode and '1MiB diagnostic bound' in oversized.stderr
cases += 1
record.write_bytes(b'\xff')
invalid = subprocess.run([sys.executable, str(program), str(record), '--bits', '8'], text=True, capture_output=True)
assert invalid.returncode and 'codec' in invalid.stderr
cases += 1
optimized = subprocess.run([sys.executable, '-O', str(program), str(record), '--bits', '8'], text=True, capture_output=True)
assert optimized.returncode and 'Assertions required' in optimized.stderr
cases += 1
print(f'PASS arithmetic returned-word parser:{cases} synthetic acceptance/refusals including every output position/malformed word,duplicate/missing/foreign/changed ideal and old partial record;NO NEW PHYSICAL EVIDENCE_OR_DEVICE_AUTHORITY')
