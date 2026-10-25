#!/usr/bin/env python3
"""Actual native bind perimeter only, not an initialization/execution grant."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 6
with tempfile.TemporaryDirectory(prefix='pioneer-output-native-binding-',dir='/private/tmp') as temporary:
    binary=compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_native_binding_test'],sys.argv[1:],producer=False)
    run_test(binary,[sys.argv[3],sys.argv[4]])
