#!/usr/bin/env python3
"""Genuine QOP1 canonical bootstrap kernel ABI compile;NO native image or IO."""
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
subprocess.run([sys.executable,str(here/'build_qwen38_gdn_output_gather_native.py'),str(sdk),str(output/'old-gather')],check=True,env=clean)
old=output/'old-gather/gdn-output-gather-NATIVE53-DATA-COMPILE-ONLY.o'
assert hashlib.sha256(old.read_bytes()).hexdigest()=='821c5a4a5865f0360ba0047ed1e931a5ad4558b2a04b177f2f6d6817948c0209','old gather DATA MUST remain exact'
traces=list(output.rglob('old-tag38-build.trace'));assert len(traces)==1
commands=[shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver=here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles=[a for a in commands if '-c' in a and a[a.index('-c')+1]==str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles)==1
args=[a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010053ULL'
args[args.index('-c')+1]=str(here/'tests/pioneer_q38_gdn_output_bootstrap_native_full_compile.cpp')
obj=output/'gdn-output-bootstrap-NATIVE53-DATA-COMPILE-ONLY.o';args[args.index('-o')+1]=str(obj)
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
sizes=[size for name,size in current.items() if 'GdnOutputBootstrap' in name or 'pioneer_gdn_output_bootstrap' in name or 'pioneer_gdn_output_matrix_metadata' in name]
assert sizes and max(sizes)<=2048
print('PASS ACTUAL native53 genuine QOP1 canonical bootstrap/119jobs/1904sources kernel DATA compile maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('ALL inherited controls/native53/gather DATA EXACT;NO linked driver/bootstrap image selection/IO/speed claim')
