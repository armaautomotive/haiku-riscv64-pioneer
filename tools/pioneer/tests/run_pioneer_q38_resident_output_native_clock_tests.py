#!/usr/bin/env python3
"""Clock injection regression and actual one-job fail containment, NO card."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test,run_test
assert len(sys.argv)==6
with tempfile.TemporaryDirectory(prefix='pioneer-output-native-clock-',dir='/private/tmp') as temporary:
    binary=compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_native_clock_test'],sys.argv[1:],producer=False)
    run_test(binary,sys.argv[3:5])
