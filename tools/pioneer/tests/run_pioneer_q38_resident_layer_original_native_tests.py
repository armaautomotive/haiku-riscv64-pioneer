#!/usr/bin/env python3
"""Actual new native lifecycle+emitted full layer with ALL original weights.

Independent CPU observer is compiled separately without native flags.
No physical card, driver install, Work fallback or token-rate claim.
"""
import os
import pathlib
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify
from pioneer_q38_resident_output_execution_common import HERE,compile_test,run_test

assert len(sys.argv)==8
image=pathlib.Path(sys.argv[6]);peer=image/'unchanged-output/exact-companion';verify(image,peer)
flags=['-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1','-DPIONEER_Q38_RESIDENT_LAYER_OWNER=1',
       '-DPIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT=1','-DPIONEER_Q38_GDN_MUTABLE_STATE=1']
with tempfile.TemporaryDirectory(prefix='pioneer-layer-original-native-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary);original=directory/'original-reference.o'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-c',str(HERE/'pioneer_q38_decoder_original_source.cpp'),'-o',str(original)],check=True)
    binary=compile_test(directory,['pioneer_q38_resident_layer_original_native_test','pioneer_q38_resident_layer_execution_machine'],
                        sys.argv[1:6],producer=False,native_projection=True,extra_flags=flags,extra_objects=[original])
    run_test(binary,[*sys.argv[1:6],str(image),str(peer),sys.argv[7]])
