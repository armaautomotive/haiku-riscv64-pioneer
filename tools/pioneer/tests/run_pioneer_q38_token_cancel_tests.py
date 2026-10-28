#!/usr/bin/env python3
"""Pinned actual cancellation parent+119 worker histories. NEVER hardware."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==5 and sys.argv[4] in ('--layer','--head','--committed')
image,old,worker=map(pathlib.Path,sys.argv[1:4]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (image,old,worker))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
for directory,script,key in ((image,here.parent/'build_qwen38_token_root.py','PINS_CANCEL'),(old,here/'run_pioneer_q38_mixed_worker_tests.py','PINS'),(worker,here.parent/'build_qwen38_global_worker_image.py','PINS')):
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
    assert pins
    for name,size,digest in pins:
        data=(directory/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory in (image,worker):
    assert subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')],text=True)==(directory/'program.symbols').read_text()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-token-cancel-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test';intervals={}
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
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
                    *flags,str(here/'pioneer_q38_token_cancel_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1','PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
