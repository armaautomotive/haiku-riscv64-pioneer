#!/usr/bin/env python3
"""Own manual-completion HOST label; delegate ALL original numeric decisions.

The label is not card provenance, native completion or Work acceptance.
Only a private snapshot loses the exact first label; raw streams stay intact.
"""
import argparse
import contextlib
import pathlib
import sys
import tempfile
import compare_pioneer_q38_token_phase_profile_log as parent
from compare_pioneer_q38_frontier_failure_report_log import stable_log
from compare_pioneer_q38_observed_trace import require

BANNER=b'Q38_MANUAL_TTSYNC_FIXED34_DIAGNOSTIC_HOST_LABEL_NOT_DEVICE_PROVENANCE_OR_WORK\n'


@contextlib.contextmanager
def snapshot(stdout,stderr):
    data=stable_log(stdout,512*1024)
    require(stable_log(stderr,0)==b'', 'manual TTSync stderr not empty')
    require(data.startswith(BANNER) and data.count(BANNER)==1, 'exact unique manual TTSync host label')
    remainder=data[len(BANNER):]
    require(remainder.startswith(parent.BANNER), 'unchanged full phase envelope required')
    with tempfile.TemporaryDirectory(prefix='pioneer-manual-ttsync-normalized-') as tmp:
        path=pathlib.Path(tmp)/'HOST-LABEL-REMOVED-NOT-RAW-CARD.log'
        path.write_bytes(remainder)
        yield path


def read_log(stdout,stderr):
    with snapshot(stdout,stderr) as path:
        return parent.read_log(path,stderr)


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu',type=pathlib.Path)
    parser.add_argument('stdout',type=pathlib.Path)
    parser.add_argument('stderr',type=pathlib.Path)
    args=parser.parse_args(argv)
    with snapshot(args.stdout,args.stderr) as path:
        print('MANUAL_TTSYNC_HOST_LABEL_ONLY_NOT_DEVICE_PROVENANCE_OR_WORK;UNCHANGED_NUMERICAL_DECISION_FOLLOWS')
        return parent.main([str(args.cpu),str(path),str(args.stderr)])


if __name__=='__main__':
    try:
        sys.exit(main())
    except (ValueError,OSError) as error:
        print('MANUAL_TTSYNC_RECORD_REFUSAL '+str(error),file=sys.stderr)
        sys.exit(2)
