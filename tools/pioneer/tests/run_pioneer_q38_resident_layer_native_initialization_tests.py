#!/usr/bin/env python3
"""NEW context's actual private zero leases/CPU-END gates; byte endpoints TEST."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 7 and sys.argv[6] in ('--all', '--positive')
flags = ['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1', '-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1',
         '-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-native-init-', dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary), ['pioneer_q38_resident_layer_native_initialization_test'],
                          sys.argv[1:6], producer=False, extra_flags=flags)
    run_test(binary, [*sys.argv[3:6], sys.argv[6]])
