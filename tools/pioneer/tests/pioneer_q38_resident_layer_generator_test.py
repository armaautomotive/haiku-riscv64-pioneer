#!/usr/bin/env python3
"""ALL13-section authentication before ANY emitted code DATA patch."""
import hashlib
import pathlib
import re
import runpy
import shutil
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv)==3
image,peer=map(pathlib.Path,sys.argv[1:])
here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_resident_layer_images.py'
module=runpy.run_path(str(generator));pins,generate=module['PINS'],module['generate']
assert len(pins)==13
count=0
for name,(section,size,digest,suffix) in pins.items():
    data=((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
    if not name.startswith('root-'):
        continue
    patch=generate([str(image),str(peer),name])
    assert patch.startswith('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayer'+suffix+'.h\n')
    assert patch.endswith('*** End Patch\n') and '// SHA256 '+digest in patch and 'DATA,NOT native/model/source/init/owner authority' in patch
    words=[int(x,16) for x in re.findall(r'0x([0-9a-f]{8})U',patch)]
    assert struct.pack('<'+'I'*len(words),*words)==data
    count+=1
with tempfile.TemporaryDirectory(prefix='pioneer-layer-generator-',dir='/private/tmp') as temporary:
    root,worker=pathlib.Path(temporary)/'root',pathlib.Path(temporary)/'peer'
    root.mkdir();worker.mkdir()
    for name,(section,_,_,_) in pins.items():
        shutil.copyfile((image if name.startswith('root-') else peer)/(section+'.bin'),(root if name.startswith('root-') else worker)/(section+'.bin'))
    for name,(section,_,_,_) in pins.items():
        target=(root if name.startswith('root-') else worker)/(section+'.bin');original=target.read_bytes()
        for changed in (bytes([original[0]^1])+original[1:],original[:-1]+bytes([original[-1]^1]),original[:-4],original+bytes(4)):
            target.write_bytes(changed)
            result=subprocess.run([sys.executable,str(generator),str(root),str(worker),'root-boot'],capture_output=True,text=True)
            assert result.returncode!=0 and not result.stdout
            count+=1
        target.write_bytes(original)
    for extra in ([],['unknown'],['peer-boot'],['root-main','EXTRA']):
        result=subprocess.run([sys.executable,str(generator),str(root),str(worker),*extra],capture_output=True,text=True)
        assert result.returncode!=0 and not result.stdout
        count+=1
assert count==63
print('PASS NEW layer generator63:ALL7ROOT exact binary roundtrips/BOTH boots+ALL13 sections authenticated BEFORE ANY output;ALL section first+last corruptions/truncation/extension/bad arity/oldpeerleaf alias refused;code DATA ONLY,no native/readwrite owner/install/Work/model or speed grant')
