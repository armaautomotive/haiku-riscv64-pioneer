#!/usr/bin/env python3
"""Real NEW native binding and ownership boundaries; no card/endpoints IO."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 6
flags = ['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1', '-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1',
         '-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-native-bind-', dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary), ['pioneer_q38_resident_layer_native_binding_test'],
                          sys.argv[1:], producer=False, extra_flags=flags)
    run_test(binary, sys.argv[3:5])
