#!/usr/bin/env python3
"""Actual complete state initialization before GPU input; no native/card."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 6
with tempfile.TemporaryDirectory(prefix='pioneer-output-initialized-input-',dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_input_initialized_test'],sys.argv[1:])
    run_test(binary,sys.argv[1:])
