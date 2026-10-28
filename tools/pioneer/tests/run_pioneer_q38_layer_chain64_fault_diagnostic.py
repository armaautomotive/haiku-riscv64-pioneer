#!/usr/bin/env python3
"""Fresh pinned legacy fault reproduction. NOT sanitizer/model qualification."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 or len(sys.argv)==4 and sys.argv[3]=='--prepare-only'
image,worker=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert image.is_absolute() and worker.is_absolute()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
for directory,script,key in ((image,here.parent/'build_qwen38_layer_chain_root.py','PINS64'),(worker,here/'run_pioneer_q38_mixed_worker_tests.py','PINS')):
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
    assert pins
    for name,size,digest in pins:
        data=(directory/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-chain64-fault-diagnostic-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test';intervals={}
    tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
    for line in subprocess.check_output([tool+'objdump','-tC',str(image/'program.elf')],text=True).splitlines():
        p=line.split(None,5)
        if len(p)!=6 or p[2]!='F':continue
        start,size=int(p[0],16),int(p[4],16)
        if not size:continue
        assert not (start|size)&3
        assert (0x40000<=start and start+size<=0x60000) or (0x64000<=start and start+size<=0x6c000)
        intervals.setdefault((start,size),p[5])
    ordered=sorted(intervals.items());end=0
    for (start,size),name in ordered:
        assert start>=end;end=start+size
    profile=pathlib.Path(tmp)/'profile.map'
    profile.write_text(''.join(format(start,'x')+' '+format(size,'x')+' '+name+'\n' for (start,size),name in ordered))
    # Deliberately faster HOST diagnostic, not a replacement for ASan/UBSan
    # qualification. The actual card ROM, interpreter semantics, RAW inputs,
    # observations and all worker histories remain the same.
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    *flags,str(here/'pioneer_q38_layer_chain64_fault_diagnostic.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
