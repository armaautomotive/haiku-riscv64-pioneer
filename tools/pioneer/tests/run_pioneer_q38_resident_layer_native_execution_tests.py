#!/usr/bin/env python3
"""Genuine full NEW native context and joined layer; simulated OS/byte endpoints."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 8 and sys.argv[7] in ('--positive', '--all')
image = pathlib.Path(sys.argv[6]); peer = image/'unchanged-output/exact-companion'
verify(image, peer)
flags = ['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1', '-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1',
         '-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-native-execute-', dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary),
                          ['pioneer_q38_resident_layer_native_execution_test', 'pioneer_q38_resident_layer_execution_machine'],
                          sys.argv[1:6], producer=False, native_projection=True, extra_flags=flags)
    run_test(binary, [*sys.argv[1:6], str(image), str(peer), sys.argv[7]])
