#!/usr/bin/env python3
"""Compare a closed tag142 diagnostic with a limited CPU trace and card baseline.

This reads saved text only. It has no device, driver, reset or Work authority.
The CPU trace used input 17 at position one, not the card's fed-back token.
Thus only position zero is comparable; the second card token is compared only
with a recorded card sequence and is not a CPU-reference quality gate.
"""
import pathlib
import subprocess
import sys


def main():
    if len(sys.argv) != 2 or not pathlib.Path(sys.argv[1]).is_absolute():
        raise ValueError('usage: analyze_qgwd_two_token_trial.py ABS_CLOSED_TRIAL_DIR')
    trial = pathlib.Path(sys.argv[1])
    if (trial / 'state').read_text() != 'CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED\n':
        raise ValueError('trial has not reported clean restoration')
    if (trial / 'trial.exit').read_text() != '0\n' or (trial / 'trial.stderr').read_bytes():
        raise ValueError('trial exit or stderr is not clean')
    log = (trial / 'trial.stdout').read_text()
    gate = pathlib.Path(__file__).with_name('pioneer_q38_qgwd_two_candidate_output.awk')
    result = subprocess.run(['awk', '-f', str(gate)], input=log, text=True,
                            capture_output=True, check=False)
    if result.returncode or result.stderr:
        raise ValueError('closed transcript failed the diagnostic shape gate')
    actual = [int(line.split()[1]) for line in log.splitlines() if line.startswith('HYBRID_TOKEN ')]
    baseline = [70253, 70253]
    print('INPUT 248319; CPU_REFERENCE_POSITION_ZERO 70253; SECOND_POSITION_NOT_COMPARABLE_CPU_INPUT_WAS_17')
    print('CARD_TOKENS ' + ' '.join(map(str, actual)))
    if actual != baseline:
        print('MISMATCH: differs from recorded card sequence; second position is NOT a CPU comparison')
        return 1
    print('MATCH: first token matches CPU trace; two-token sequence matches recorded card sequence; not quality or speed proof')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, UnicodeError, ValueError) as exc:
        print('ERROR: ' + str(exc), file=sys.stderr)
        sys.exit(2)
