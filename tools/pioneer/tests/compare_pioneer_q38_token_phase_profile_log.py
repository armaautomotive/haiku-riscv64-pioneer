#!/usr/bin/env python3
"""Strict held-phase DATA reader plus UNCHANGED frontier34/reference checks.

Reports device-cycle fractions only. No assumed clock frequency, hardware
utilization, throughput extrapolation, runtime CPU inference or device IO.
The exact new envelope alone is removed in a private normalized snapshot;
raw logs, old parser, original trace pins and numerical decisions are unchanged.
"""
import argparse
from fractions import Fraction
import os
import pathlib
import re
import sys
import tempfile
from compare_pioneer_q38_frontier_failure_report_log import stable_log
from compare_pioneer_q38_frontier_benchmark_log import read_log as legacy_read_log
from compare_pioneer_q38_observed_trace import load_cpu, require
from compare_pioneer_q38_decode_benchmark_log import compare_first_three

BANNER = b'Q38_TOKEN_PHASE_FIXED34_HELD_READ_NO_CPU_REFERENCE_NOT_WORK_RAW_CYCLES_NOT_FREQUENCY\n'
ROW = re.compile(rb'Q38_TOKEN_PHASE_PACKET word([0-9]{2})((?: [0-9a-f]{8}){8})\n')
TRAILER = re.compile(rb'Q38_TOKEN_PHASE_AFTER_CLEAN_END words96 valid([01]);ALL34_WITH_BOOTSTRAP_STOP_AND_OBSERVER_COST_RAW_CYCLES_NOT_FREQUENCY_OR_WORK\n')
PHASES = ('Bootstrap', 'Metadata', 'Recurrent', 'Dense', 'HeadNormalize',
          'HeadMatrix', 'Feedback', 'StateCommit', 'Publication', 'FinalStop')


def u64(words, at):
    return words[at] | words[at + 1] << 32


def closed_metric(words):
    """Mirror the pure DATA Closed() grammar, NOT a model acceptance check."""
    if len(words) != 60 or words[:5] != [0x31485051, 1, 60, 2, 9] or not 1 <= words[12] <= 8192:
        return False
    if any(words[13:16]) or any(words[46:]) or u64(words, 9) != u64(words, 5) or u64(words, 9) <= u64(words, 7):
        return False
    cycles = segments = 0
    for phase in range(10):
        at = 16 + phase * 3
        expected = 1 if phase in (0, 9) else words[12] * (64 if phase == 1 else 48 if phase == 2 else 16 if phase == 3 else 1)
        delta = u64(words, at + 1)
        if words[at] != expected or not delta:
            return False
        cycles += delta
        segments += words[at]
        if cycles > (1 << 64) - 1 or segments > (1 << 32) - 1:
            return False
    return segments == words[11] and cycles == u64(words, 9) - u64(words, 7)


def decode_packet(words, declared):
    require(len(words) == 96 and all(0 <= w < 1 << 32 for w in words), 'phase packet length/word domain')
    require(words[:8] == [1, 1, 0, 0, 0, 0, 0, 0], 'phase immutable request')
    require(words[8:16] == [1, 1, 0, 1, 1, 0, 0, 0], 'phase success/ready/safe reply')
    raw = words[32:92]
    valid = closed_metric(raw) and raw[12] == 34
    expected = [1, 1, 4, 0, 59, 0, 1, int(valid), 34, 60, raw[3], raw[13], 0, 0, 0, 0]
    require(words[16:32] == expected and words[23] == declared and not any(words[92:]),
            'phase detail/raw/declared-valid/padding mismatch')
    # Invalid telemetry can accompany a real clean END. Preserve its exact raw
    # DATA, but NEVER derive fractions or claim a valid profile from it.
    result = {'valid': valid, 'raw_words': tuple(raw), 'buckets': []}
    if valid:
        total = u64(raw, 9) - u64(raw, 7)
        result['total_cycles'] = total
        for phase, name in enumerate(PHASES):
            at = 16 + phase * 3
            delta = u64(raw, at + 1)
            result['buckets'].append((name, raw[at], delta, Fraction(delta, total)))
    return result


def read_log(stdout, stderr):
    data = stable_log(stdout, 512 * 1024)
    require(stable_log(stderr, 0) == b'' and not os.path.samefile(stdout, stderr), 'stderr is not empty/distinct')
    require(data.startswith(BANNER), 'not the exact full-token phase banner')
    lines = data[len(BANNER):].splitlines(keepends=True)
    require(len(lines) >= 14, 'missing phase envelope/body')
    trailer = TRAILER.fullmatch(lines[-1])
    require(trailer is not None, 'phase trailer')
    words = []
    for row, line in enumerate(lines[-13:-1]):
        match = ROW.fullmatch(line)
        require(match is not None and int(match[1]) == row * 8, 'phase packet row/order/format')
        words.extend(int(word, 16) for word in match[2].split())
    profile = decode_packet(words, int(trailer[1]))
    # Only these 14 exact new envelope lines are removed. All complete upload,
    # record, token-feedback, END and count/rate checks remain the old parser.
    with tempfile.TemporaryDirectory(prefix='pioneer-phase-normalized-') as temporary:
        normalized = pathlib.Path(temporary) / 'NORMALIZED-DATA-NOT-RAW-HARDWARE.log'
        normalized.write_bytes(b''.join(lines[:-13]))
        records, rate = legacy_read_log(normalized)
    return records, rate, profile


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('stdout', type=pathlib.Path)
    parser.add_argument('stderr', type=pathlib.Path)
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute(), 'absolute CPU trace directory required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role override')
    cpu = load_cpu(args.cpu)
    records, rate, profile = read_log(args.stdout, args.stderr)
    print('PASS TOKEN_PHASE envelope/whole96word reply/UNCHANGED FRONTIER34 grammar;NOT provenance,model acceptance or Work')
    print('OBSERVED_COUNT_WINDOW ' + str(rate) if rate else 'RATE_UNAVAILABLE')
    if profile['valid']:
        print('DEVICE_CYCLE_PROFILE total' + str(profile['total_cycles'])
              + ';ALL34_PLUS_BOOTSTRAP_STOP_OBSERVER_COST_NOT_32TOKEN_RATE_WINDOW_NOT_PURE_ENGINE_UTILIZATION')
        for name, count, cycles, fraction in profile['buckets']:
            print(f'DEVICE_CYCLE_BUCKET {name} segments{count} cycles{cycles} fraction{fraction}')
    else:
        print('INVALID_PHASE_TELEMETRY_CLEAN_END_RETAINED_NO_CYCLE_FRACTIONS')
    # ALL original CPU pins load even for invalid telemetry. Numerical failure
    # retains exit1; valid envelope/timing never overrides an arithmetic failure.
    if not compare_first_three(cpu, records):
        return 1
    return 0 if profile['valid'] else 2


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('TOKEN_PHASE_RECORD_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
