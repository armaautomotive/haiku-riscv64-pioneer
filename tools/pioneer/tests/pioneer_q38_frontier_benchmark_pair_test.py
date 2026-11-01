#!/usr/bin/env python3
"""SYNTHETIC paired-rate tests;NEVER measured generation or hardware timing."""
import contextlib
from fractions import Fraction
import io
import os
import pathlib
import subprocess
import sys
import tempfile
import compare_pioneer_q38_frontier_benchmark_log as legacy
import compare_pioneer_q38_frontier_benchmark_pair as pair

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
# The entire unchanged strict parser/control suite still runs FIRST.
subprocess.run([sys.executable, str(here / 'pioneer_q38_frontier_benchmark_comparator_test.py')], check=True)


def records():
    result = []
    for position in range(legacy.POSITIONS):
        record = [0] * 64
        record[:12] = [0x31435451, 1, position,
                       248319 if position == 0 else 17 if position == 1 else 99 + position,
                       100 + position, 0x3f800000, position + 1, (position + 1) & 1,
                       int(position == 0), int(position < legacy.POSITIONS - 1), 64, 973]
        record[12:36] = list(range(112, 136))
        record[44:47] = [position + 1, 48, 119]
        result.append(record)
    return result


def fixture(values=None, shortest=319999999, longest=320000001, first=2, unavailable=False):
    values = records() if values is None else values
    lines = [legacy.HEADER, legacy.OPEN, f'Q38_FRONTIER_UPLOADED {legacy.TOTAL_BYTES}/{legacy.TOTAL_BYTES}',
             legacy.START, legacy.TERMINAL]
    for position, record in enumerate(values):
        for row in range(8):
            lines.append(f'Q38_FRONTIER_RECORD position{position} word{row * 8:02d}'
                         + ''.join(f' {word:08x}' for word in record[row * 8:row * 8 + 8]))
    tokens = legacy.POSITIONS - first
    rate = (f'Q38_FRONTIER_OBSERVED_COUNT_RATE_BOUNDS sampled_tokens{tokens} first_committed{first}'
            f' last_committed34 elapsed_min_us{shortest} elapsed_max_us{longest}'
            f' lower_tokens_per_sec{tokens * 1000000 / longest:.9f}'
            f' upper_tokens_per_sec{tokens * 1000000 / shortest:.9f} observations1000;'
            'POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF')
    return '\n'.join(lines + [legacy.END, legacy.UNAVAILABLE if unavailable else rate]) + '\n'


accepted = refused = 0
with tempfile.TemporaryDirectory(prefix='pioneer-frontier-pair-SYNTHETIC-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    old, new = root / 'previous-SYNTHETIC.log', root / 'candidate-SYNTHETIC.log'
    old.write_text(fixture())

    def check(text, okay, expected=None):
        global accepted, refused
        new.write_text(text)
        try:
            result = pair.compare(old, new)
        except ValueError:
            assert not okay
            refused += 1
            return
        assert okay
        if expected is not None:
            assert result[2] == expected, result
        accepted += 1

    low = Fraction(100 * (319999999 - 160000001), 160000001)
    high = Fraction(100 * (320000001 - 159999999), 159999999)
    check(fixture(shortest=159999999, longest=160000001), True, (low, high))
    old_rate, new_rate, gain = pair.compare(new, old)
    assert gain == (100 * (Fraction(159999999, 320000001) - 1),
                    100 * (Fraction(160000001, 319999999) - 1))
    accepted += 1
    # Rounded displayed rates coincide here;raw count/time bounds do NOT.
    check(fixture(), True, (Fraction(-200, 320000001), Fraction(200, 319999999)))
    check(fixture(shortest=639999999, longest=640000001), True,
          (100 * (Fraction(319999999, 640000001) - 1),
           100 * (Fraction(320000001, 639999999) - 1)))
    old.write_text(fixture(shortest=320000000, longest=320000000))
    check(fixture(shortest=320000000, longest=320000000), True, (Fraction(0), Fraction(0)))
    old.write_text(fixture())
    values = records()
    for record in values:
        record[12:36] = list(range(212, 236))
        record[38] = record[42] = 0xffffffff
    check(fixture(values), True)
    for position in range(legacy.POSITIONS):
        values = records()
        values[position][5] ^= 1  # valid finite score,different arithmetic header
        check(fixture(values), False)
    # A separately valid token-feedback path is still not a matched pair.
    values = records()
    values[0][4] += 1
    check(fixture(values), False)
    check(fixture(first=3), False)
    check(fixture(unavailable=True), False)
    for text in ('', legacy.HEADER + '\n', fixture()[:-1].rsplit('\n', 1)[0] + '\n',
                 fixture().replace('lower_tokens_per_sec0.100000000', 'lower_tokens_per_sec0.200000000'),
                 fixture().replace(legacy.END, legacy.END + '\n' + legacy.END)):
        check(text, False)
    new.write_text(fixture())
    for paths in ((old, old), (pathlib.Path('relative.log'), new), (old, root / 'missing.log')):
        try:
            pair.compare(*paths)
        except (ValueError, OSError):
            refused += 1
        else:
            raise AssertionError(paths)
    # CLI containment cannot silently acquire native/Work/CPU role overrides.
    for key in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            run = subprocess.run([sys.executable, str(pathlib.Path(pair.__file__)), str(old), str(new)],
                                 env={**os.environ, key: value}, text=True, capture_output=True)
            assert run.returncode == 2 and 'unexpected role override' in run.stderr
            refused += 1
    with contextlib.redirect_stdout(io.StringIO()) as output:
        assert pair.main([str(old), str(new)]) == 0
    assert 'exact=[' in output.getvalue() and 'strict CPU comparator remains separate' in output.getvalue()
    accepted += 1
print(f'PASS paired frontier34/{accepted} SYNTHETIC positives/{refused} header/window/partial/rate/path/role refusals;'
      'exact rational raw-count bounds/legacy strict suite unchanged;NO device IO,hardware timing,quality or Work proof')
