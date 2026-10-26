#!/usr/bin/env python3
"""Actual DEBUG69 public control/native lifetime;TEST endpoints,NO card."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import HERE,compile_test,run_test

if len(sys.argv) not in (7,8):
    raise SystemExit(2)
kernel=HERE.parents[2]/'src/add-ons/kernel/drivers/misc'
image=pathlib.Path(sys.argv[6]);peer=image/'exact-companion'
from pioneer_q38_resident_layer_image_common import verify
peer=image/'unchanged-output/exact-companion';verify(image,peer)
role=''.join((kernel/name).read_text() for name in ('PioneerQ38ResidentLayerValidationRole.h','PioneerQ38ResidentLayerOwnerRole.h','PioneerQ38ResidentLayerNativeRole.h'))
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==31
flags=['-D'+name+'=1' for name in sorted(parents)]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026102600010069ULL']
native=(kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-output-debug69-control-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary)
    for marker,name in (('PIONEER_PROBE_LOCK','debug69_probe_lock.inc'),('PIONEER_Q38_GDN_CALL_SCOPE','debug69_call_scope.inc')):
        begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert native.count(begin)==native.count(end)==1
        code=native.split(begin,1)[1].split(end,1)[0]
        if marker=='PIONEER_PROBE_LOCK':
            # This SAME CPU global already belongs to the genuine native
            # fixture. Keep the actual locking/check functions unchanged.
            declaration='static const void* sQ38LocalDeltaOwner=NULL; // sLock;cleared ONLY after clean END'
            assert code.count(declaration)==1;code=code.replace(declaration,'')
        (directory/name).write_text(code)
    start=native.index('static status_t\nProbeControl(')
    end=native.index('// PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE_ROUTING_END',start)
    (directory/'debug69_routing.inc').write_text(native[start:end]+'\n#endif\n}\n')
    objects=[]
    if len(sys.argv)==8:
        # Independent original GGUF/libm observer is a SEPARATE CPU-only
        # TU,with NONE of the native adapter/owner/image/Work flags.
        original=directory/'original-reference.o'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-c',str(HERE/'pioneer_q38_decoder_original_source.cpp'),'-o',str(original)],check=True)
        objects.append(original);flags.append('-DPIONEER_Q38_TEST_RESIDENT_LAYER_ORIGINAL=1')
    binary=compile_test(directory,['pioneer_q38_resident_layer_validation_control_test','pioneer_q38_resident_layer_execution_machine'],
                        sys.argv[1:6],producer=False,native_projection=True,extra_flags=flags,extra_objects=objects)
    run_test(binary,[*sys.argv[1:6],str(image),str(peer),*sys.argv[7:]])
