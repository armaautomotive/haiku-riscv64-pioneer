#!/usr/bin/env python3
"""HOST-only original layer29/new wide emitted image diagnostic;never native/Work."""
import pathlib
import sys
from compare_pioneer_q38_observed_trace import require
from run_pioneer_q38_layer29_replay import main

if __name__=='__main__':
    require(len(sys.argv)==5,'MODEL CPU_TRACE_DIRECTORY PARTIAL_NATIVE_DIRECTORY WIDE_DELTA13_IMAGE_DIRECTORY')
    image=pathlib.Path(sys.argv.pop())
    raise SystemExit(main(wide_emitted=image))
