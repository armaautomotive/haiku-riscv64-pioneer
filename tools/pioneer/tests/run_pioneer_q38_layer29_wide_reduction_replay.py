#!/usr/bin/env python3
"""HOST-only partial-FMA compensated-reduction experiment; never native/Work."""
import sys
from compare_pioneer_q38_observed_trace import require
from run_pioneer_q38_layer29_replay import main

if __name__=='__main__':
    require(len(sys.argv)==4, 'MODEL CPU_TRACE_DIRECTORY PARTIAL_NATIVE_DIRECTORY')
    raise SystemExit(main(wide=True))
