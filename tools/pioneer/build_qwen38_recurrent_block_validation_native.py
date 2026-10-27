#!/usr/bin/env python3
"""Build separate physical DEBUG72 module;NO install/device/Work action."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
qualified,out=map(pathlib.Path,sys.argv[1:]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent;saved=sys.argv[:]
try:
 sys.argv=[str(here/'tests/run_pioneer_q38_recurrent_block_validation_native_compile.py'),str(qualified),str(out/'sdk')]
 compiled=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
legacy=compiled['base']['base'];obj=compiled['obj'];args=compiled['args']
old_object=legacy['original'][legacy['original'].index('-o')+1]
links=[line for line in legacy['commands'] if '-shared' in line and old_object in line];assert len(links)==1
link=links[0][:]
assert link[:4]==[args[0],'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2]=='-o' and len(link[4:-2])==7
assert [pathlib.Path(path).name for path in link[4:-2]]==[
 'crtbeginS.o','haiku_version_glue.o','tenstorrent_pci_probe.o','PioneerPciProfile.o','kernel.so','libgcc.a','crtendS.o']
assert all(pathlib.Path(path).is_file() for path in link[4:-2])
module=out/'tenstorrent_pci_probe-recurrent-DEBUG72'
link=[str(obj) if arg==old_object else arg for arg in link];link[-1]=str(module)
subprocess.run(link,check=True,env=legacy['clean'])
tool=pathlib.Path(args[0]).parent
symbols=subprocess.check_output([str(tool/'riscv64-unknown-haiku-nm'),'--defined-only',str(module)],text=True)
names={row[-1] for row in map(str.split,symbols.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for forbidden in ('OriginalSource','ReferenceProducts','ValidationSession','Q38WorkControl','Q38MixedLiveValidationControl',
 'PioneerQ38MixedLiveNativeState','pioneer_q38_recurrent_context_only'):
 assert forbidden not in symbols,forbidden
print(hashlib.sha256(module.read_bytes()).hexdigest()+'  '+str(module))
print('PASS linked conventional ABI DEBUG72/namespace23/fixed20800B;NO private CPU oracle,Work dispatcher,old dense handler or NODRV ABI alias')
print('BUILD ONLY;actual runtime/handler/client/finalEND and ORIGINAL physical comparison REQUIRED BEFORE use;NOT full model or speed')
