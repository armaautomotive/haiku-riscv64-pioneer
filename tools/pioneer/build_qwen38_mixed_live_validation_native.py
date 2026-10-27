#!/usr/bin/env python3
"""Build separate physical DEBUG70 module. NO install/device/Work action."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
qualified,out=map(pathlib.Path,sys.argv[1:])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
 sys.argv=[str(here/'tests/run_pioneer_q38_mixed_live_validation_native_compile.py'),str(qualified),str(out/'sdk')]
 compiled=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
legacy=compiled['base']['base'];obj=compiled['obj'];args=compiled['args']
old_object=legacy['original'][legacy['original'].index('-o')+1]
links=[a for a in legacy['commands'] if '-shared' in a and old_object in a];assert len(links)==1
link=links[0][:]
assert link[:4]==[args[0],'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2]=='-o' and len(link[4:-2])==7
assert [pathlib.Path(p).name for p in link[4:-2]]==[
 'crtbeginS.o','haiku_version_glue.o','tenstorrent_pci_probe.o','PioneerPciProfile.o','kernel.so','libgcc.a','crtendS.o']
assert all(pathlib.Path(p).is_file() for p in link[4:-2])
module=out/'tenstorrent_pci_probe-mixed-dense-DEBUG70'
link=[str(obj) if a==old_object else a for a in link];link[-1]=str(module)
subprocess.run(link,check=True,env=legacy['clean'])
tool=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(tool/'riscv64-unknown-haiku-nm'),'--defined-only',str(module)],text=True)
names={row[-1] for row in map(str.split,symbols.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for word in ('OriginalSource','CompareFinal','ReferenceProducts','ValidationSession','pioneer_q38_live_context_only',
 'Q38ResidentLayerValidationControl','Q38WorkControl','PioneerQ38ResidentLayerOwner'):
 assert word not in symbols,word
print(hashlib.sha256(module.read_bytes()).hexdigest()+'  '+str(module))
print('PASS linked conventional ABI DEBUG70/namespace21/fixed20736B;NO old Work dispatcher/CPU observer/test/NODRV entry alias')
print('BUILD ONLY;public handler/client containment and ORIGINAL physical comparison REQUIRED BEFORE use;NOT full model or speed')
