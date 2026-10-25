#!/usr/bin/env python3
"""Actual separate DEBUG68 driver/control ABI compile;NO link/install/card."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith('PIONEER_') and v!='0' for k,v in os.environ.items()):
    raise SystemExit(2)
qualified,output=map(pathlib.Path,sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
output.mkdir(mode=0o700)
saved=list(sys.argv)
try:
    # Reuse the exact authenticated compiler trace/SDK/old65 and inert-entry
    # audits,not a second hand-maintained kernel include/flag approximation.
    sys.argv=[str(here/'run_pioneer_q38_resident_output_native_context_compile.py'),str(qualified),str(output/'context67-control')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
args=list(base['args'])
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010068ULL'
args[args.index('-c')+1]=str(here/'pioneer_q38_resident_output_validation_native_full_compile.cpp')
obj=output/'resident-output-debug68-UNLINKED.o'
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True,env=base['clean'])
current,old=base['frames'](obj.with_suffix('.su')),base['previous']
for name,size in current.items():
    assert size<=2048 or name in old and size<=old[name],(name,size,old.get(name))
new=[size for name,size in current.items() if 'ResidentOutput' in name or 'pioneer_output_debug_' in name]
assert new and max(new)<=2048
for required in ('Q38ResidentOutputValidationControl(','ProbeControl('):
    concrete=[size for name,size in current.items() if required in name]
    assert concrete and max(concrete)<=2048,(required,'actual control/routing must be bounded and emitted')
toolchain=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-nm'),'--defined-only',str(obj)],text=True)
names={line.split()[-1] for line in symbols.splitlines() if line.split()}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
        'pioneer_output_debug_control','pioneer_output_debug_route'}<=names
assert not any(name.startswith('pioneer_q38_output_context_only_') for name in names),'DEBUG68 cannot borrow renamed inert67 entries'
print('PASS ACTUAL separate DEBUG68 complete kernel/context/IO/control/closed routing/owner/END compile;maxNewFrame='+str(max(new)))
print('PASS conventional driver ABI definitions present,NO compile-only67 entry aliases;context67 entries remain independently proven inert/old65 BYTE-IDENTICAL')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('UNLINKED object ONLY;NO physical/original-GGUF numeric qualification,device/install/Work/model-speed grant')
