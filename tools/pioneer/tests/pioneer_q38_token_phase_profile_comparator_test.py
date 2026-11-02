#!/usr/bin/env python3
"""SYNTHETIC timing envelopes; never card measurements or quality evidence."""
import contextlib
from fractions import Fraction
import io
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile
import compare_pioneer_q38_token_phase_profile_log as candidate
import compare_pioneer_q38_frontier_benchmark_log as legacy

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
# The old F8, complete frontier and exact-rational rate tests remain unchanged.
parent = runpy.run_path(str(here / 'pioneer_q38_frontier_failure_report_comparator_test.py'))
fixture, records = parent['fixture'], parent['records']
accepted = refused = 0


def packet():
    raw = [0] * 60
    raw[:5] = [0x31485051, 1, 60, 2, 9]
    raw[12] = 34
    total = 0
    for phase in range(10):
        count = 1 if phase in (0, 9) else 34 * (64 if phase == 1 else 48 if phase == 2 else 16 if phase == 3 else 1)
        cycles = count * (phase + 1) * 19
        raw[16 + phase * 3:19 + phase * 3] = [count, cycles, 0]
        raw[11] += count
        total += cycles
    begin = (1 << 40) + 7
    for at, value in ((5, begin + total), (7, begin), (9, begin + total)):
        raw[at:at + 2] = [value & 0xffffffff, value >> 32]
    return ([1, 1] + [0] * 6 + [1, 1, 0, 1, 1, 0, 0, 0]
            + [1, 1, 4, 0, 59, 0, 1, 1, 34, 60, 2, 0, 0, 0, 0, 0] + raw + [0] * 4)


def envelope(words=None, body=None, declared=None):
    words = packet() if words is None else words
    body = fixture().encode('ascii') if body is None else body
    declared = words[23] if declared is None else declared
    rows = b''.join(f'Q38_TOKEN_PHASE_PACKET word{at:02d}'.encode('ascii')
                    + b''.join(f' {w:08x}'.encode('ascii') for w in words[at:at + 8]) + b'\n'
                    for at in range(0, 96, 8))
    trailer = (f'Q38_TOKEN_PHASE_AFTER_CLEAN_END words96 valid{declared};'
               'ALL34_WITH_BOOTSTRAP_STOP_AND_OBSERVER_COST_RAW_CYCLES_NOT_FREQUENCY_OR_WORK\n').encode('ascii')
    return candidate.BANNER + body + rows + trailer


with tempfile.TemporaryDirectory(prefix='pioneer-phase-SYNTHETIC-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    stdout, stderr = root / 'SYNTHETIC.stdout', root / 'SYNTHETIC.stderr'

    def check(data, okay, error=b'', available=True, metric=True):
        global accepted, refused
        stdout.write_bytes(data)
        stderr.write_bytes(error)
        before = stdout.read_bytes(), stderr.read_bytes()
        try:
            actual, rate, profile = candidate.read_log(stdout, stderr)
        except (ValueError, OSError):
            assert not okay
            refused += 1
        else:
            assert okay and actual == records() and (rate is not None) == available and profile['valid'] == metric
            if metric:
                assert len(profile['buckets']) == 10 and sum(b[3] for b in profile['buckets']) == Fraction(1)
                assert sum(b[2] for b in profile['buckets']) == profile['total_cycles']
            else:
                assert not profile['buckets'] and 'total_cycles' not in profile
            accepted += 1
        assert before == (stdout.read_bytes(), stderr.read_bytes()), 'RAW_LOG_REWRITTEN'

    original = envelope()
    check(original, True)
    check(envelope(body=fixture(unavailable=True).encode('ascii')), True, available=False)
    bad_metric = packet()
    bad_metric[23] = 0
    bad_metric[35] = bad_metric[26] = 3  # sticky INVALID, not a closed profile
    bad_metric[45] = bad_metric[27] = 1  # raw+detail clock error
    check(envelope(bad_metric), True, metric=False)
    check(envelope(bad_metric, declared=1), False)
    check(envelope(declared=0), False)
    # Compare Python packet grammar against the ACTUAL shared C++ ReplyShape.
    cases = [packet(), bad_metric]
    expected = [True, True]
    for word in range(96):
        changed = packet()
        changed[word] ^= 1
        cases.append(changed)
        expected.append(False)
        check(envelope(changed, declared=1), False)
    binary = root / 'packet-data'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_token_phase_profile_packet_data.cpp'),
                    '-o', str(binary)], check=True)
    run = subprocess.run([str(binary)], input=''.join(' '.join(map(str, row)) + '\n' for row in cases),
                         text=True, capture_output=True, check=True)
    assert not run.stderr and list(map(int, run.stdout.splitlines())) == list(map(int, expected))
    accepted += len(cases)
    for at in range(len(candidate.BANNER)):
        changed = bytearray(original)
        changed[at] ^= 1
        check(bytes(changed), False)
    for data in (b'', fixture().encode('ascii'), candidate.BANNER, candidate.BANNER + original,
                 original + b'EXTRA\n', original[:-1], original.replace(b'\n', b'\r\n'),
                 original.replace(b'word08', b'word00'), original.replace(b'words96', b'words95')):
        check(data, False)
    lines = original.splitlines(keepends=True)
    for at in range(1, len(lines)):
        check(b''.join(lines[:at] + lines[at + 1:]), False)
        check(b''.join(lines[:at] + [lines[at]] + lines[at:]), False)
    for error in (b' ', b'\n', b'FAILED_IOCTL\n', bytes(range(256))):
        check(original, False, error)
    for old, new in ((b'clean_end1', b'clean_end0'), (b'final_count34', b'final_count33'),
                     (b'lower_tokens_per_sec0.100000000', b'lower_tokens_per_sec0.200000000')):
        check(original.replace(old, new), False)
    for paths in ((pathlib.Path('relative'), stderr), (stdout, pathlib.Path('relative')),
                  (root / 'missing', stderr), (stdout, root / 'missing'), (stdout, stdout)):
        try:
            candidate.read_log(*paths)
        except (ValueError, OSError):
            refused += 1
        else:
            raise AssertionError('invalid file identity accepted')
    stdout.write_bytes(original)
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
    # The new envelope must preserve strict failure even when timing is valid
    # or invalid. Use unchanged numerical comparator, never a tolerance change.
    actual, _, _ = candidate.read_log(stdout, stderr)
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
    # Exercise the real CLI decision path with synthetic pinned-reader input,
    # not a runtime CPU model or changed reference/production comparator.
    saved_loader = candidate.load_cpu
    candidate.load_cpu = lambda path: cpu
    try:
        for valid in (True, False):
            for mismatch in (False, True):
                values = records()
                if mismatch:
                    values[0][5] ^= 1
                stdout.write_bytes(envelope(None if valid else bad_metric, fixture(values).encode('ascii')))
                with contextlib.redirect_stdout(io.StringIO()) as output:
                    status = candidate.main([str(root), str(stdout), str(stderr)])
                assert status == (1 if mismatch else 0 if valid else 2)
                assert ('DEVICE_CYCLE_BUCKET ' in output.getvalue()) == valid
                accepted += 1
    finally:
        candidate.load_cpu = saved_loader
    for key in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            run = subprocess.run([sys.executable, str(pathlib.Path(candidate.__file__)), str(root), str(stdout), str(stderr)],
                                 env={**os.environ, key: value}, text=True, capture_output=True)
            assert run.returncode == 2 and 'unexpected role override' in run.stderr
            refused += 1
print(f'PASS token-phase comparator/{accepted} SYNTHETIC positives/{refused} envelope,stderr,packet,file,complete-record,rate,role,numeric refusals;'
      '98 ACTUAL C++ packet grammar cross-checks/old parser+reference decisions UNCHANGED;NO hardware,frequency,model speed or Work proof')
