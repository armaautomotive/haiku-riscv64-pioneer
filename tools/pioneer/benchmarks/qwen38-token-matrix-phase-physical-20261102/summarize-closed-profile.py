#!/usr/bin/env python3
"""Posthoc summary of the strict reader's CLOSED latest-worker-job DATA.

No IO to Pioneer. No changed reference, tolerance, inference or Work grant.
Fractions are descriptive per-job statistics, never parallel wall time.
"""
import argparse
import collections
from fractions import Fraction
import json
import os
import pathlib
import statistics
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('reader_root', type=pathlib.Path)
    p.add_argument('cpu', type=pathlib.Path)
    p.add_argument('stdout', type=pathlib.Path)
    p.add_argument('stderr', type=pathlib.Path)
    a = p.parse_args()
    if not all(x.is_absolute() for x in (a.reader_root, a.cpu, a.stdout, a.stderr)):
        raise ValueError('absolute paths required')
    if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
        raise ValueError('unexpected role override')
    sys.path.insert(0, str(a.reader_root / 'tools/pioneer/tests'))
    import compare_pioneer_q38_token_matrix_phase_log as reader
    cpu = reader.load_cpu(a.cpu)
    records, rate, coarse, profile = reader.read_log(a.stdout, a.stderr)
    print('STRICT_CLOSED_LOG_SHAPE_PASS_NOT_NUMERICAL_ACCEPTANCE')
    grouped = collections.defaultdict(list)
    for sample in profile['samples']:
        if sample['valid']:
            grouped[sample['inputs'], sample['groups']].append(sample)
    result = {
        'scope': 'LATEST_COMPLETED_Q4_JOB_PER_WORKER_ONLY_INCLUSIVE_OBSERVER_AND_WAITS_NOT_WHOLE_MODEL_OR_PURE_ENGINE_UTILIZATION_OR_FREQUENCY',
        'states_missing_invalid_clock_intervals_malformed': profile['states'],
        'all_worker_metrics_valid': profile['all_valid'],
        'post_prompt_observation_rate': rate,
        'coarse_phase_valid': coarse['valid'],
        'groups': [],
    }
    for (inputs, groups), samples in sorted(grouped.items()):
        row = {
            'K': inputs, 'groups': groups, 'worker_samples': len(samples),
            'worker_ids': [s['worker'] for s in samples],
            'latest_individual_job_inclusive_cycle_min': min(s['total_cycles'] for s in samples),
            'latest_individual_job_inclusive_cycle_max': max(s['total_cycles'] for s in samples),
            'descriptive_bucket_statistics_not_wall_time': [],
        }
        for index, name in enumerate(reader.BUCKETS):
            values = [s['buckets'][index][2] for s in samples]
            for value in values:
                if not isinstance(value, Fraction) or not 0 < value < 1:
                    raise ValueError('invalid strict-reader fraction')
            row['descriptive_bucket_statistics_not_wall_time'].append({
                'bucket': name,
                'minimum_fraction': str(min(values)),
                'median_fraction': str(statistics.median(values)),
                'maximum_fraction': str(max(values)),
                'minimum_percent': float(min(values) * 100),
                'median_percent': float(statistics.median(values) * 100),
                'maximum_percent': float(max(values) * 100),
            })
        result['groups'].append(row)
    print(json.dumps(result, indent=2))
    # Same actual original reader decision, not an approximate substitute.
    if not reader.compare_first_three(cpu, records):
        return 1
    return 0 if profile['all_valid'] and coarse['valid'] else 2


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('CLOSED_PROFILE_SUMMARY_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
