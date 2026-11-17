#!/usr/bin/env python3
"""Compare two closed raw-Q4 rate transcripts; no device access or authority."""
from decimal import Decimal
from hashlib import sha256
import json
from pathlib import Path
import sys


sys.path.insert(0, str(Path(__file__).resolve().parent / 'tests'))
from parse_pioneer_q38_full_offload_rate_log import InvalidLog, parse_complete, require, stable_text


EXPECTED_ANSWER_HEX = '310a320a330a340a350a360a370a380a390a31300a31310a31320a31330a3134'


def read_run(stage):
    for filename in ('rate.exit', 'parent.exit'):
        require(stable_text(stage / filename) == '0\n', filename + ' must be exactly zero')
    stdout = stable_text(stage / 'rate.stdout')
    result = parse_complete(stdout, stable_text(stage / 'rate.stderr'))
    require(result['rate_available'] and result['output_tokens'] == 32 and result['prompts'] == 31,
            'both trials need the complete raw-Q4 31-prompt/32-output request')
    require(result['answer_hex'] == EXPECTED_ANSWER_HEX, 'answer differs from the measured raw-Q4 control')
    return sha256(stdout.encode('ascii')).hexdigest(), result['observed_post_prompt_count_rate_bounds']


def compare(serial, batched):
    require(serial.resolve() != batched.resolve(), 'two independent run directories are required')
    (serial_sha, reference), (batched_sha, candidate) = map(read_run, (serial, batched))
    require(serial_sha != batched_sha, 'duplicate trial transcript')
    geometry = ('sampled_commit_steps', 'first_committed', 'last_committed')
    require(all(reference[field] == candidate[field] for field in geometry), 'mismatched commit windows')
    # Recompute from integer elapsed endpoints, not the rounded TPS fields.
    minimum = Decimal(reference['minimum_us']) / candidate['maximum_us']
    maximum = Decimal(reference['maximum_us']) / candidate['minimum_us']
    comparison = ('candidate_faster_in_observed_pair' if minimum > 1 else
                  'candidate_slower_in_observed_pair' if maximum < 1 else 'timing_bounds_overlap')
    return {
        'serial_tps': [reference['lower_tps'], reference['upper_tps']],
        'batched_tps': [candidate['lower_tps'], candidate['upper_tps']],
        'transcript_sha256': {'serial': serial_sha, 'batched': batched_sha},
        'candidate_over_serial_percent_change_bounds': [f'{(r-1)*100:.9f}' for r in (minimum, maximum)],
        'window': {field: reference[field] for field in geometry},
        'both_match_reference_answer': True,
        'result': comparison,
        'scope': 'one matched post-prompt count window per binary; excludes weight upload and START',
        'limitation': 'timing bounds only, not repeatability, isolated latency or broad quality',
        'requires_independent_package_identity_and_machine_postflight': True,
    }


def main():
    if len(sys.argv) != 3:
        raise SystemExit('use CLOSED_SERIAL_CAPTURE CLOSED_BATCHED_CAPTURE')
    try:
        result = compare(*map(Path, sys.argv[1:]))
    except (OSError, InvalidLog, ValueError) as error:
        raise SystemExit('REFUSE_HEAD_CSR_RATE_PAIR ' + str(error))
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
