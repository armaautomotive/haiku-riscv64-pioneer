#!/usr/bin/env python3
"""SYNTHETIC F8 envelope tests;NEVER accelerator/timing/quality evidence."""
import contextlib
import io
import os
import pathlib
import runpy
import tempfile
import compare_pioneer_q38_frontier_failure_report_log as candidate
import compare_pioneer_q38_frontier_benchmark_log as legacy

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
# ALL unchanged old strict-parser AND exact-rational paired controls FIRST.
parent = runpy.run_path(str(here / 'pioneer_q38_frontier_benchmark_pair_test.py'))
fixture, records = parent['fixture'], parent['records']
accepted = refused = 0
with tempfile.TemporaryDirectory(prefix='pioneer-frontier-F8-SYNTHETIC-') as temporary:
    root = pathlib.Path(temporary)
    stdout, stderr = root / 'SYNTHETIC.stdout', root / 'SYNTHETIC.stderr'

    def check(data, okay, error=b'', available=True):
        global accepted, refused
        stdout.write_bytes(data)
        stderr.write_bytes(error)
        before = stdout.read_bytes(), stderr.read_bytes()
        try:
            actual, rate = candidate.read_log(stdout, stderr)
        except (ValueError, OSError):
            assert not okay
            refused += 1
        else:
            assert okay and actual == records() and (rate is not None) == available
            accepted += 1
        assert before == (stdout.read_bytes(), stderr.read_bytes()), 'RAW_LOG_REWRITTEN'

    original = fixture().encode('ascii')
    valid = candidate.BANNER + original
    check(valid, True)
    check(candidate.BANNER + fixture(unavailable=True).encode('ascii'), True, available=False)
    for data in (b'', original, candidate.BANNER, valid + b'UNEXPECTED\n',
                 candidate.BANNER + valid, candidate.BANNER[:-1] + original,
                 candidate.BANNER.replace(b'\n', b'\r\n') + original):
        check(data, False)
    for at in range(len(candidate.BANNER)):
        changed = bytearray(valid)
        changed[at] ^= 1
        check(bytes(changed), False)
    lines = original.splitlines(keepends=True)
    for at in range(len(lines)):
        check(candidate.BANNER + b''.join(lines[:at] + lines[at + 1:]), False)
        check(candidate.BANNER + b''.join(lines[:at] + [lines[at]] + lines[at:]), False)
    for error in (b' ', b'\n', b'Q38_FRONTIER_IOCTL_FAILED\n', b'Q38_FRONTIER_FAILED_REPLY_RAW\n',
                  b'Q38_FRONTIER_FAILED_PACKET word00 00000000\n', bytes(range(256))):
        check(valid, False, error)
    for old, new in ((b'clean_end1', b'clean_end0'), (b'final_count34', b'final_count33'),
                     (b'lower_tokens_per_sec0.100000000', b'lower_tokens_per_sec0.200000000')):
        check(valid.replace(old, new), False)
    check(valid.replace(b'Q38_FRONTIER_UPLOADED ', b'Q38_FRONTIER_FAILED_REPLY_RAW '), False)
    for paths in ((pathlib.Path('relative'), stderr), (stdout, pathlib.Path('relative')),
                  (root / 'missing', stderr), (stdout, root / 'missing'), (stdout, stdout)):
        try:
            candidate.read_log(*paths)
        except (ValueError, OSError):
            refused += 1
        else:
            raise AssertionError('invalid file identity accepted')
    stdout.write_bytes(valid)
    stderr.write_bytes(b'')
    link = root / 'linked'
    for target in (stdout, stderr):
        link.symlink_to(target)
        try:
            candidate.read_log(link if target == stdout else stdout, link if target == stderr else stderr)
        except ValueError:
            refused += 1
        else:
            raise AssertionError('symlink accepted')
        link.unlink()
    # Valid envelope does not change the original numerical decision.
    actual, rate = candidate.read_log(stdout, stderr)
    cpu = [{'header': [0, 0, pos, row[3], row[4], 0, row[5]]} for pos, row in enumerate(actual[:3])]
    with contextlib.redirect_stdout(io.StringIO()):
        assert legacy.compare_first_three(cpu, actual)
        accepted += 1
        for pos in range(3):
            for word in (3, 4, 5):
                changed = [row[:] for row in actual]
                changed[pos][word] ^= 1
                assert not legacy.compare_first_three(cpu, changed)
                refused += 1
print(f'PASS F8_DIAGNOSTIC comparator/{accepted} SYNTHETIC positives/{refused} envelope+stderr+file+complete-grammar+numeric refusals;old parser/expectations UNCHANGED;NO physical/TPS/Work proof')
