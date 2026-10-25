#!/usr/bin/env python3
"""Build the distinct DEBUG68 driver;NO installation,device access or Work grant."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith('PIONEER_') and v!='0' for k,v in os.environ.items()):
    raise SystemExit(2)
qualified,output=map(pathlib.Path,sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
output.mkdir(mode=0o700)
saved=list(sys.argv)
try:
    # The complete old65/inert67/DEBUG68 SDK and frame audits must pass
    # FIRST. Never turn the renamed refusing67 object into a driver.
    sys.argv=[str(here/'tests/run_pioneer_q38_resident_output_validation_native_compile.py'),
              str(qualified),str(output/'compile-controls')]
    compiled=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
base=compiled['base'];args=list(compiled['args'])
roles=(kernel/'PioneerQ38ResidentOutputValidationRole.h').read_text()+(kernel/'PioneerQ38ResidentOutputOwnerRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',roles))
assert len(parents)==28
for name in sorted(parents):
    key='-D'+name
    args=[a for a in args if a!=key and not a.startswith(key+'=')]+[key+'=1']
assert [a for a in args if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]==['-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010068ULL']
obj=output/'resident-output-debug68.o'
# Compile the ACTUAL production TU,not the test TU's exported witnesses.
args[args.index('-c')+1]=str(kernel/'tenstorrent_pci_probe.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True,env=base['clean'])
current=base['frames'](obj.with_suffix('.su'))
for name,size in current.items():
    assert size<=2048 or name in base['previous'] and size<=base['previous'][name],(name,size)
new=[size for name,size in current.items() if 'ResidentOutput' in name or 'ProbeControl(' in name]
assert new and max(new)<=2048
toolchain=pathlib.Path(args[0]).parent
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
def names(path):
    table=subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-nm'),'--defined-only',str(path)],text=True)
    return {row[-1] for row in map(str.split,table.splitlines()) if row}
defined=names(obj)
assert normal<=defined
assert not any(n.startswith(('pioneer_q38_output_context_only_','pioneer_output_context_','pioneer_output_debug_')) for n in defined)
original=base['original'];old_object=original[original.index('-o')+1]
links=[a for a in base['commands'] if '-shared' in a and old_object in a]
assert len(links)==1
link=list(links[0])
assert link[:4]==[args[0],'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2]=='-o' and len(link[4:-2])==7
assert [pathlib.Path(p).name for p in link[4:-2]]==[
    'crtbeginS.o','haiku_version_glue.o','tenstorrent_pci_probe.o','PioneerPciProfile.o','kernel.so','libgcc.a','crtendS.o']
assert all(pathlib.Path(p).is_file() for p in link[4:-2])
candidate=output/'tenstorrent_pci_probe-resident-output-DEBUG68'
link=[str(obj) if a==old_object else a for a in link]
link[-1]=str(candidate)
subprocess.run(link,check=True,env=base['clean'])
defined=names(candidate)
assert normal<=defined and not any(n.startswith(('pioneer_q38_output_context_only_','pioneer_output_context_','pioneer_output_debug_')) for n in defined)
print('PASS ACTUAL production DEBUG68/namespace19/fixed6516736-byte diagnostic/closed10-operation dispatcher;NO test-witness or inert67 ABI;maxNewFrame='+str(max(new)))
print(hashlib.sha256(candidate.read_bytes()).hexdigest()+'  '+str(candidate))
print('BUILT ONLY;original-GGUF/full native failure containment/physical qualification REQUIRED BEFORE INSTALL;NO Work/token-loop/inference-speed grant')
