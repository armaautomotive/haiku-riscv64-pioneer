#!/usr/bin/env python3
"""Actual private producer and complete layer owner; simulated endpoints only."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 8 and sys.argv[7] in [str(i) for i in range(21)] + ['--all', '--preflight-faults']
image = pathlib.Path(sys.argv[6]); peer = image/'unchanged-output/exact-companion'
verify(image, peer)
flags = ['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1',
         '-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
         '-DPIONEER_Q38_TEST_RESIDENT_LAYER_EXECUTION=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-owner-execution-', dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary),
                          ['pioneer_q38_resident_layer_owner_execution_test', 'pioneer_q38_resident_layer_execution_machine'],
                          sys.argv[1:6], extra_flags=flags)
    run_test(binary, [*sys.argv[1:6], str(image), str(peer), sys.argv[7]])
