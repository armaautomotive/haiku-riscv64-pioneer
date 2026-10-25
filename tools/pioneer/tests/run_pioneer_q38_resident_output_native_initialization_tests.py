#!/usr/bin/env python3
"""Actual new native initialization factories/scopes and emitted zero, NO card."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test,run_test

assert len(sys.argv)==7 and sys.argv[6] in ('--all','--positive')
with tempfile.TemporaryDirectory(prefix='pioneer-output-native-init-',dir='/private/tmp') as temporary:
    binary=compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_native_initialization_test'],sys.argv[1:6],producer=False)
    run_test(binary,[*sys.argv[3:6],sys.argv[6]])
