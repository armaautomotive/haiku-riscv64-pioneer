#!/usr/bin/env python3
"""Compile/inspect NEW physical diagnostic object;NEVER link/install/invoke."""
import hashlib
import pathlib
import re
import runpy
import subprocess
import sys
assert len(sys.argv)==3
qualified,out=map(pathlib.Path,sys.argv[1:])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
saved=sys.argv[:]
try:
 sys.argv=[str(here/'run_pioneer_q38_mixed_live_native_context_compile.py'),str(qualified),str(out/'unchanged-nodrv'),'--final-release']
 base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
role=(kernel/'PioneerQ38MixedLiveValidationRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==23
# The historical trace selected an entire OLD Work image/dispatcher chain.
# None is this physical diagnostic. Start from ONLY the new role's explicit
# PIONEER_Q38 parents;old controls above keep their original flags/images.
args=[a for a in base['args'] if a!='-DTT_PIONEER_Q38_WORK_MODE'
 and not a.startswith(('-DTT_PIONEER_Q38_WORK_MODE=','-DPIONEER_Q38_'))]
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102700010070ULL'
obj=out/'mixed-live-validation70.o'
args[args.index('-c')+1]=str(kernel/'tenstorrent_pci_probe.cpp')
args[args.index('-o')+1]=str(obj)
for name in sorted(parents):
 if not any(a=='-D'+name or a.startswith('-D'+name+'=') for a in args):args+=['-D'+name+'=1']
subprocess.run(args,check=True,env=base['base']['clean'])
frames=base['base']['frames'](obj.with_suffix('.su'))
previous=base['frames']
for name,size in frames.items():
 assert size<=2048 or name in previous and size<=previous[name],(name,size,previous.get(name))
for name in ('Q38MixedLiveValidationControl(', 'MixedLiveBaseScope(', 'MixedLiveFinalReleaseScope(',
 'PioneerQ38MixedLiveOwner::Run(', 'PioneerQ38MixedLiveOwner::ValidationReadback('):
 assert any(name in n for n in frames),(name,'actual control/controller must instantiate,not constant refusal')
tool=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(tool/'riscv64-unknown-haiku-nm'),'--defined-only',str(obj)],text=True)
names={row[-1] for row in map(str.split,symbols.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for forbidden in ('CompareFinal','OriginalSource','ReferenceProducts','Q38ResidentLayerValidationControl',
 'pioneer_q38_live_context_only','PioneerQ38ResidentLayerOwner'):
 assert forbidden not in symbols,forbidden
selected=[size for name,size in frames.items() if any(n in name for n in ('MixedLive','mixed_live'))]
print('PASS actual DEBUG70 conventional ABI/control/native scopes/embedding+RUN+readback+END SDK object;maxNewFrame'+str(max(selected)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('COMPILE/INSPECTION ONLY;not linked,installed,invoked,physical-qualified or a Work/inference speed claim')
