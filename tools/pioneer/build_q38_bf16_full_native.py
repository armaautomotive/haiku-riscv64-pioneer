#!/usr/bin/env python3
"""Build/inspect separate QTP4 full native driver;NEVER install or run."""
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[1]
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_full_offload_native.py'),str(sdk),str(out/'unchanged-native')]
 old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
tool=old['tool'];cxx=old['cxx'];common=old['common'];includes=old['includes']
flags=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
 ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')]
flags+=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010105ULL']
driver=root/'src/add-ons/kernel/drivers/misc';commands=[]
dependencyText=subprocess.check_output([*common,*flags,*includes,'-MM',str(driver/'pioneer_q38_bf16_full_driver.cpp')],text=True)
dependencies={Path(p).resolve() for p in dependencyText.replace('\\\n','').split(':',1)[1].split() if Path(p).resolve().is_relative_to(root)}
dependencies.update((Path(__file__).resolve(),here/'build_qwen38_full_offload_native.py'))
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(args):
 commands.append(list(map(str,args)));subprocess.run(list(map(str,args)),check=True,cwd=out)
run([*common,*flags,*includes,'-MMD','-MF',out/'native.d','-c',driver/'pioneer_q38_bf16_full_driver.cpp','-o',out/'native.o'])
run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',old['compiler_path']('crtbeginS.o'),
 old['release']/'system/glue/haiku_version_glue.o',out/'native.o',out/'unchanged-native/profile.o',
 old['release']/'system/kernel/kernel.so',old['compiler_path']('libgcc.a'),old['compiler_path']('crtendS.o'),
 '-o',out/'tenstorrent_pci_probe-bf16'])
symbols=subprocess.check_output([tool+'nm','-C','--defined-only',str(out/'tenstorrent_pci_probe-bf16')],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(out/'tenstorrent_pci_probe-bf16')],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
 'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in symbols,name
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO','NativeQ38PeerInstructionModel'):
 assert name not in symbols and name not in undefined,name
raw=subprocess.check_output([tool+'nm','-SC','--defined-only',str(out/'native.o')],text=True)
for name in ('RootBf16','PeerBf16','PeerFpu'):
 assert re.search(r'\bPioneerQ38TokenBf16Programs::'+name+r'$',raw,re.M),name
assert 'PioneerQ38TokenPushedPrograms::' not in raw and 'PioneerQ38FullOffloadPrograms::' not in raw
frames=[line.split('\t') for line in (out/'native.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
(out/'native.symbols').write_text(symbols);(out/'native.undefined').write_text(undefined)
(out/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
for path in (out/'native.o',out/'tenstorrent_pci_probe-bf16'):
 print(path.name,path.stat().st_size,hashlib.sha256(path.read_bytes()).hexdigest(),flush=True)
print('PASS BUILD_ONLY distinct tag105/QTP4 complete BF16 ROOT+QGW4 native driver/all15 code sections/'
 'new61778152B arbitrary-original journal/no CPU backend/unchanged tag88 and default pins/maxFrame'+str(max(int(row[1]) for row in frames))+
 ';NO installed driver/lifecycle/full-model quality/hardware/TPS qualification',flush=True)
