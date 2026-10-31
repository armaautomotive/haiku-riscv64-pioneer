#!/usr/bin/env python3
"""Synthetic DATA-log parser/comparator tests. NEVER accelerator results."""
import contextlib
import io
import os
import pathlib
import sys
import tempfile

import compare_pioneer_q38_token_validation_log as checked

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
assert len(sys.argv) in (1, 2)
cpu_path = (pathlib.Path(sys.argv[1]) if len(sys.argv) == 2 else
            pathlib.Path(__file__).resolve().parents[3]/'generated.pioneer/qualification-inputs/cpu-original-trace-20261028')
assert cpu_path.is_absolute()
cpu = checked.load_cpu(cpu_path)
# Intentionally synthesize DATA from the pinned oracle to test comparison;
# this is NEVER archived as observed model execution or a physical pass.
records = []
for pos, trace in enumerate(cpu):
    h = trace['header']
    r = [0]*64
    r[:12] = [0x31435451, 1, pos, h[3], h[4], h[6], pos+1, (pos+1)&1, int(pos == 0), int(pos < 2), 64, 973]
    r[12:36] = list(range(24))
    r[44:47] = [pos+1, 48, 119]
    records.append(r)
def log(values):
    rows = [checked.HEADER, checked.OPEN, f'DEBUG74_UPLOADED {checked.TOTAL_BYTES}/{checked.TOTAL_BYTES}']
    for pos, r in enumerate(values):
        for row in range(8):
            rows.append(f'DEBUG74_RECORD position{pos} word{row*8:02d}'+''.join(f' {word:08x}' for word in r[row*8:row*8+8]))
    return '\n'.join([*rows, checked.END])+'\n'
positive = refusals = 0
with tempfile.TemporaryDirectory(prefix='pioneer-debug74-DATA-log-', dir='/private/tmp') as temporary:
    path = pathlib.Path(temporary)/'SYNTHETIC-NOT-MODEL.log'
    def check(data, expected):
        global positive, refusals
        path.write_text(data)
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                result = checked.main([str(cpu_path), str(path)])
        except ValueError:
            result = 2
        assert result == expected, (result, expected)
        if result == 0:
            positive += 1
        else:
            refusals += 1
    valid = log(records)
    check(valid, 0)
    for pos in range(3):
        for word in (0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 35, 38, 44, 45, 46, 47, 63):
            changed = [r[:] for r in records]
            changed[pos][word] ^= 1
            check(log(changed), 1 if word == 5 else 2)
    for pos in range(3):
        changed = [r[:] for r in records]
        changed[pos][4] ^= 1
        if pos == 1:
            changed[2][3] = changed[1][4]  # well-formed but numerically wrong feedback
        check(log(changed), 1)
    rows = valid.splitlines()
    for changed in (rows[:-1], rows[:-2]+[rows[-1]], rows[:3]+[rows[3]]+rows[3:],
                    rows[:3]+rows[4:]+[rows[3]], [rows[0], rows[1], *rows[3:]],
                    [rows[0], rows[1], rows[2].replace(str(checked.TOTAL_BYTES), '4096'), *rows[3:]],
                    [rows[0], rows[1], rows[2], rows[2], *rows[3:]],
                    [rows[0], 'PASS_DEBUG74_SOURCE_ONLY', *rows[2:]],
                    rows[:-1]+[rows[-1].replace('clean_end1', 'clean_end0')],
                    rows+['FAIL_DEBUG74'], ['UNTRUSTED PREFIX', *rows]):
        check('\n'.join(changed)+'\n', 2)
print(f'PASS DEBUG74 posthoc DATA comparator/{positive} synthetic positive/{refusals} malformed+missing+duplicate+order+source-only+input+greedy+score refusals/pinned independent CPU controls;'
      'NO genuine generated token,native,physical,Work or TPS proof')
