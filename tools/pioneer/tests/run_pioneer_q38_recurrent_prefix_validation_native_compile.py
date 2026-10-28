#!/usr/bin/env python3
"""Build/inspect distinct physical DEBUG73 object;NEVER install/run a device."""
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
 sys.argv=[str(here/'run_pioneer_q38_recurrent_prefix_native_context_compile.py'),str(qualified),str(out/'unchanged-nodrv'),'--control']
 base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
role=(kernel/'PioneerQ38RecurrentPrefixValidationRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==22
args=[a for a in base['args'] if a!='-DTT_PIONEER_Q38_WORK_MODE'
 and not a.startswith(('-DTT_PIONEER_Q38_WORK_MODE=','-DPIONEER_Q38_'))]
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102800010073ULL';obj=out/'recurrent-prefix-validation73.o'
args[args.index('-c')+1]=str(kernel/'tenstorrent_pci_probe.cpp');args[args.index('-o')+1]=str(obj)
for name in sorted(parents):
 if not any(a=='-D'+name or a.startswith('-D'+name+'=') for a in args):args+=['-D'+name+'=1']
subprocess.run(args,check=True,env=base['base']['clean'])
frames=base['base']['frames'](obj.with_suffix('.su'));assert frames
for name,size in frames.items():assert size<=2048 or name in base['frames'] and size<=base['frames'][name],(name,size)
for name in ('Q38RecurrentPrefixValidationControl(', 'Q38RecurrentPrefixValidationEnd(', 'RecurrentPrefixBaseScope(',
 'RecurrentPrefixFinalReleaseScope(', 'InitializeQ38RecurrentPrefixState(', 'InitializeQ38RecurrentPrefixHistory(',
 'PioneerQ38RecurrentPrefixOwner::Run(', 'PioneerQ38RecurrentPrefixOwner::ValidationReadback('):
 assert any(name in key for key in frames),(name,'actual complete body must instantiate')
tool=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(tool/'riscv64-unknown-haiku-nm'),'--defined-only',str(obj)],text=True)
names={row[-1] for row in map(str.split,symbols.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for forbidden in ('OriginalSource','ReferenceProducts','CompareFinal','Q38WorkControl','Q38MixedLiveValidationControl',
 'Q38RecurrentBlockValidationControl','PioneerQ38RecurrentBlockNativeState','pioneer_q38_recurrent_prefix_context_body'):
 assert forbidden not in symbols,forbidden
selected=[size for name,size in frames.items() if 'RecurrentPrefix' in name or 'recurrent_prefix' in name]
print('PASS actual DEBUG73 conventional ABI/closed prefix routing/control/SAME private QPE1+672GDN+336history+3RUN+readback+post-END-usercopy SDK object;maxNewFrame'+str(max(selected)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' '+obj.name)
print('COMPILE/INSPECTION ONLY;NOT linked,installed,invoked,physical/runtime/routing qualified,full-model Work or speed proof')
