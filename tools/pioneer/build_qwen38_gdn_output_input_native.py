#!/usr/bin/env python3
"""Kernel ABI compile of genuine op6 input descriptors only;NO linked driver."""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys
if any(k.startswith('PIONEER_') and v!='0' for k,v in os.environ.items()):
    raise SystemExit(2)
if len(sys.argv)!=3:
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output')
here=pathlib.Path(__file__).resolve().parent
sdk,output=map(pathlib.Path,sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
subprocess.run([sys.executable,str(here/'build_qwen38_gdn_output_producer_native.py'),str(sdk),str(output/'old53')],check=True,env=clean)
old=output/'old53/gdn-output-producer-NATIVE53-COMPILE-ONLY.o'
assert hashlib.sha256(old.read_bytes()).hexdigest()=='c30cd92274945b4664fd71a379ac74d3f034330b530b0948d1d366852485fafb','old53 MUST remain exact'
traces=list(output.rglob('old-tag38-build.trace'));assert len(traces)==1
commands=[shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver=here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles=[a for a in commands if '-c' in a and a[a.index('-c')+1]==str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles)==1
args=[a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010053ULL'
args[args.index('-c')+1]=str(here/'tests/pioneer_q38_gdn_output_input_native_full_compile.cpp')
obj=output/'gdn-output-input-NATIVE53-DATA-COMPILE-ONLY.o';args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True,env=clean)
def frames(path):
    result={}
    for line in path.read_text().splitlines():
        fields=line.split('\t')
        if len(fields)!=3:
            continue
        match=re.fullmatch(r'(.*?):\d+:\d+:(.*)',fields[0]);assert match
        name=str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[name]=max(result.get(name,0),int(fields[1]))
    return result
current,previous=frames(obj.with_suffix('.su')),frames(old.with_suffix('.su'));assert current
for name,size in current.items():
    assert size<=2048 or name in previous and size<=previous[name],(name,size,previous.get(name))
sizes=[size for name,size in current.items() if 'InputDescriptor' in name or 'pioneer_gdn_output_input' in name]
assert sizes and max(sizes)<=2048
print('PASS ACTUAL native53 SAME-owner/EXACT cohort/ALL24descriptor DATA kernel compile maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('ALL old controls through native53 EXACT;NO linked driver/consumer/image selector/device IO/speed claim')
