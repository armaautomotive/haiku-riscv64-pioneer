#!/usr/bin/env python3
"""All-six/both-boots pin-before-output and generated-header byte checks."""
import hashlib
import pathlib
import re
import runpy
import shutil
import struct
import subprocess
import sys
import tempfile

here=pathlib.Path(__file__).resolve().parent.parent
generator=here/'generate_qwen38_next_gdn_output_images.py'
pins=runpy.run_path(str(generator))['PINS']
assert len(sys.argv)==3
root,peer=map(pathlib.Path,sys.argv[1:]);cases=0
headers=here.parents[1]/'src/add-ons/kernel/drivers/misc'
for name,(section,size,pin,suffix) in pins.items():
    data=((root if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==pin
    text=(headers/('PioneerQ38NextGdnOutput'+suffix+'.h')).read_text()
    words=[int(w,16) for w in re.findall(r'0x([0-9a-f]{8})U',text)]
    assert struct.pack('<'+'I'*len(words),*words)==data
    result=subprocess.run([sys.executable,str(generator),str(root),str(peer),name],capture_output=True,text=True,timeout=5)
    assert not result.returncode and result.stdout.startswith('*** Begin Patch\n*** Add File:') and result.stdout.endswith('*** End Patch\n')
    lines=result.stdout.splitlines()
    assert all(line.startswith('+') for line in lines[2:-1])
    assert '\n'.join(line[1:] for line in lines[2:-1])+'\n'==text
    cases+=1
with tempfile.TemporaryDirectory(prefix='pioneer-next-gdn-output-generator-') as temporary:
    base=pathlib.Path(temporary);a=base/'root';b=base/'peer';a.mkdir();b.mkdir()
    for source,dest in ((root,a),(peer,b)):
        for path in source.glob('*.bin'):
            shutil.copyfile(path,dest/path.name)
    for kind,path in (('root',a),('peer',b)):
        files=['boot.bin']+[section+'.bin' for name,(section,_,_,_) in pins.items() if name.startswith(kind+'-')]
        for filename in files:
            target=path/filename;original=target.read_bytes()
            variants=[original[:-1],original+b'\0']
            for offset in (0,len(original)-1):
                bad=bytearray(original);bad[offset]^=1;variants.append(bad)
            for bad in variants:
                target.write_bytes(bad)
                # Even an UNSELECTED peer/helper/boot corruption prevents ANY
                # patch from being printed. No partial image admission.
                result=subprocess.run([sys.executable,str(generator),str(a),str(b),'root-main'],capture_output=True,text=True,timeout=5)
                assert result.returncode and not result.stdout
                target.write_bytes(original);cases+=1
    for selection in ('','old-QPS1','../root-main'):
        result=subprocess.run([sys.executable,str(generator),str(a),str(b),selection],capture_output=True,text=True,timeout=5)
        assert result.returncode and not result.stdout;cases+=1
print(f'PASS complete QOP2/QOI2 generated headers EXACT/all-six BOTH-boot early rejection cases{cases};NO native registry/map/IO grant')
