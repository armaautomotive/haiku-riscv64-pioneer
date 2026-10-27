#!/usr/bin/env python3
"""Compile/inspect a distinct physical diagnostic object;NEVER install/run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
qualified,out=map(pathlib.Path,sys.argv[1:]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
saved=sys.argv[:]
try:
 sys.argv=[str(here/'run_pioneer_q38_recurrent_block_native_context_compile.py'),str(qualified),str(out/'unchanged-nodrv'),'--readback']
 base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
role=(kernel/'PioneerQ38RecurrentBlockValidationRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==22
args=[a for a in base['args'] if a!='-DTT_PIONEER_Q38_WORK_MODE'
 and not a.startswith(('-DTT_PIONEER_Q38_WORK_MODE=','-DPIONEER_Q38_'))]
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102700010071ULL';obj=out/'recurrent-block-validation71.o'
args[args.index('-c')+1]=str(kernel/'tenstorrent_pci_probe.cpp');args[args.index('-o')+1]=str(obj)
for name in sorted(parents):
 if not any(a=='-D'+name or a.startswith('-D'+name+'=') for a in args):args+=['-D'+name+'=1']
subprocess.run(args,check=True,env=base['base']['clean'])
frames=base['base']['frames'](obj.with_suffix('.su'));assert frames
for name,size in frames.items():assert size<=2048 or name in base['frames'] and size<=base['frames'][name],(name,size)
for name in ('Q38RecurrentBlockValidationControl(', 'RecurrentBlockBaseScope(', 'RecurrentBlockFinalReleaseScope(',
 'InitializeQ38RecurrentBlockState(', 'InitializeQ38RecurrentBlockHistory(', 'PioneerQ38RecurrentBlockOwner::Run(',
 'PioneerQ38RecurrentBlockOwner::ValidationReadback('):
 assert any(name in key for key in frames),(name,'actual complete handler/controller must instantiate')
tool=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(tool/'riscv64-unknown-haiku-nm'),'--defined-only',str(obj)],text=True)
names={row[-1] for row in map(str.split,symbols.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for forbidden in ('OriginalSource','ReferenceProducts','CompareFinal','Q38WorkControl','Q38MixedLiveValidationControl',
 'PioneerQ38MixedLiveNativeState','pioneer_q38_recurrent_context_only'):
 assert forbidden not in symbols,forbidden
selected=[size for name,size in frames.items() if 'RecurrentBlock' in name or 'recurrent_block' in name]
print('PASS actual DEBUG71 conventional ABI/control/native scopes/QPE1+672GDN+336history+RUN+readback+finalEND SDK object;maxNewFrame'+str(max(selected)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('COMPILE/INSPECTION ONLY;NOT linked,installed,invoked,public-handler/runtime/client/physical qualified or a full-model/Work/speed claim')
