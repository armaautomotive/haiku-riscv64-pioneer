#!/usr/bin/env python3
"""Actual private QPS1 input does not replace initialized full-layer state."""
import pathlib
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import compile_test, run_test

assert len(sys.argv) == 6
flags = ['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1',
         '-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-owner-preflight-', dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary), ['pioneer_q38_resident_layer_owner_preflight_test'],
                          sys.argv[1:], resident_states=False, extra_flags=flags)
    run_test(binary, sys.argv[1:])
