#!/usr/bin/env python3
"""Strict posthoc F8 diagnostic DATA reader;not hardware or Work acceptance.

Only the exact failure-report client's one informational banner is removed.
Every remaining byte goes through the UNCHANGED frontier34 parser and the
UNCHANGED first-three CPU comparison. Any stderr rejects a clean result.
The raw input files are never rewritten;no device IO or runtime CPU model.
"""
import argparse
import os
import pathlib
import stat
import sys
import tempfile
from compare_pioneer_q38_frontier_benchmark_log import read_log as legacy_read_log
from compare_pioneer_q38_observed_trace import load_cpu, require
from compare_pioneer_q38_decode_benchmark_log import compare_first_three

BANNER = b'Q38_FRONTIER_FAILURE_REPORT_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_REPLY_NOT_ROOT_CAUSE_OR_CLEAN_END\n'


def stable_log(path, limit):
    require(path.is_absolute() and not path.is_symlink(), 'absolute nonsymlink diagnostic log required')
    with path.open('rb') as source:
        before = os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and 0 <= before.st_size <= limit, 'diagnostic log type/length')
        data = source.read(limit + 1)
        after = os.fstat(source.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        require(identity(before) == identity(after) and len(data) == before.st_size, 'diagnostic log changed')
    return data


def read_log(stdout, stderr):
    data = stable_log(stdout, 512 * 1024)
    require(stable_log(stderr, 0) == b'' and not os.path.samefile(stdout, stderr), 'stderr is not empty/distinct')
    require(data.startswith(BANNER), 'not the exact F8 diagnostic banner')
    # A private bounded byte snapshot,NOT a new observed hardware artifact.
    # No arbitrary line stripping,failed-packet filtering or grammar changes.
    with tempfile.TemporaryDirectory(prefix='pioneer-frontier-F8-normalized-') as temporary:
        normalized = pathlib.Path(temporary) / 'NORMALIZED-DATA-NOT-RAW-HARDWARE.log'
        normalized.write_bytes(data[len(BANNER):])
        return legacy_read_log(normalized)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('stdout', type=pathlib.Path)
    parser.add_argument('stderr', type=pathlib.Path)
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute(), 'absolute CPU trace directory required')
    require(not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ), 'role override')
    cpu = load_cpu(args.cpu)
    records, rate = read_log(args.stdout, args.stderr)
    print('PASS F8_DIAGNOSTIC exact sole banner/empty stderr/UNCHANGED FRONTIER34 DATA+rate grammar;NOT hardware provenance or numerical acceptance')
    print('OBSERVED_COUNT_WINDOW ' + str(rate) if rate else 'RATE_UNAVAILABLE')
    return 0 if compare_first_three(cpu, records) else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('F8_DIAGNOSTIC_RECORD_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
