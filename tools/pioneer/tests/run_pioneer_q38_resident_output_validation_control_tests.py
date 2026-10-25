#!/usr/bin/env python3
"""Actual DEBUG68 public control/native lifetime;TEST endpoints,NO card."""
import hashlib
import pathlib
import re
import runpy
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import HERE,compile_test,run_test

if len(sys.argv)!=7:
    raise SystemExit(2)
kernel=HERE.parents[2]/'src/add-ons/kernel/drivers/misc'
image=pathlib.Path(sys.argv[6]);peer=image/'exact-companion'
for name,(section,size,digest,_) in runpy.run_path(str(HERE.parent/'generate_qwen38_resident_output_images.py'))['PINS'].items():
    data=((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
role=(kernel/'PioneerQ38ResidentOutputValidationRole.h').read_text()+(kernel/'PioneerQ38ResidentOutputOwnerRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==28
flags=['-D'+name+'=1' for name in sorted(parents)]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010068ULL']
native=(kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-output-debug68-control-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary)
    for marker,name in (('PIONEER_PROBE_LOCK','debug68_probe_lock.inc'),('PIONEER_Q38_GDN_CALL_SCOPE','debug68_call_scope.inc')):
        begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert native.count(begin)==native.count(end)==1
        code=native.split(begin,1)[1].split(end,1)[0]
        if marker=='PIONEER_PROBE_LOCK':
            # This SAME CPU global already belongs to the genuine native
            # fixture. Keep the actual locking/check functions unchanged.
            declaration='static const void* sQ38LocalDeltaOwner=NULL; // sLock;cleared ONLY after clean END'
            assert code.count(declaration)==1;code=code.replace(declaration,'')
        (directory/name).write_text(code)
    start=native.index('static status_t\nProbeControl(')
    end=native.index('// PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE_ROUTING_END',start)
    (directory/'debug68_routing.inc').write_text(native[start:end]+'\n#endif\n}\n')
    binary=compile_test(directory,['pioneer_q38_resident_output_validation_control_test','pioneer_q38_resident_output_execution_machine'],
                        sys.argv[1:6],producer=False,native_projection=True,extra_flags=flags)
    run_test(binary,[*sys.argv[1:6],str(image),str(peer)])
