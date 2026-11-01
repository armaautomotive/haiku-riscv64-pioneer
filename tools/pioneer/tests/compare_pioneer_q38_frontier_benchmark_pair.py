#!/usr/bin/env python3
"""Posthoc paired-log arithmetic ONLY;no device IO or CPU model inference.

Each input must already be a complete frontier34 log. Same client, clock,
isolation, model identity and hardware provenance need independent evidence;
this tool cannot establish those facts from log text. It does not replace the
unchanged strict CPU comparator or establish repeatability, quality or Work.
"""
import argparse
from fractions import Fraction
import os
import pathlib
import sys
from compare_pioneer_q38_frontier_benchmark_log import read_log
from compare_pioneer_q38_observed_trace import require

WINDOW = ('sampled_tokens', 'first_committed', 'last_committed')


def bounds(rate):
    """Use exact raw integer counts/times,not rounded printed token rates."""
    return (Fraction(rate['sampled_tokens'] * 1000000, rate['elapsed_max_us']),
            Fraction(rate['sampled_tokens'] * 1000000, rate['elapsed_min_us']))


def compare(previous, candidate):
    require(previous.is_absolute() and candidate.is_absolute(), 'absolute log paths required')
    require(not os.path.samefile(previous, candidate), 'distinct log files required;not independent-run proof')
    old_records, old_rate = read_log(previous)
    new_records, new_rate = read_log(candidate)
    changed = [position for position, (old, new) in enumerate(zip(old_records, new_records))
               if old[:12] != new[:12]]
    require(not changed, 'paired first12 record words differ at positions ' + str(changed))
    require(old_rate is not None and new_rate is not None, 'both completed rate windows required')
    require(all(old_rate[key] == new_rate[key] for key in WINDOW), 'different paired count windows')
    old_lower, old_upper = bounds(old_rate)
    new_lower, new_upper = bounds(new_rate)
    gain = (100 * (new_lower / old_upper - 1), 100 * (new_upper / old_lower - 1))
    return old_rate, new_rate, gain


def exact(value):
    return str(value.numerator) + '/' + str(value.denominator)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('previous', type=pathlib.Path)
    parser.add_argument('candidate', type=pathlib.Path)
    args = parser.parse_args(argv)
    require(not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ),
            'unexpected role override')
    old, new, gain = compare(args.previous, args.candidate)
    print('PAIRED_LOG_DATA_ONLY all34 first12 record words EXACT;session nonces/physical counters NOT compared across runs')
    print('MATCHED_COUNT_WINDOW ' + ' '.join(key + '=' + str(old[key]) for key in WINDOW))
    for name, rate in (('PREVIOUS', old), ('CANDIDATE', new)):
        low, high = bounds(rate)
        print(name + ' exact_tokens_per_second=[' + exact(low) + ',' + exact(high)
              + '] approximate=[' + format(float(low), '.12f') + ',' + format(float(high), '.12f')
              + '] observations=' + str(rate['observations']))
    print('ONE_PAIR_GAIN_PERCENT exact=[' + exact(gain[0]) + ',' + exact(gain[1])
          + '] approximate=[' + format(float(gain[0]), '.12f') + ',' + format(float(gain[1]), '.12f') + ']')
    print('NOT hardware/client/clock/isolation provenance,causal attribution,repeatability,full-model numerical acceptance,sustained inference or Assistant Work;strict CPU comparator remains separate')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('FRONTIER34_PAIR_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
