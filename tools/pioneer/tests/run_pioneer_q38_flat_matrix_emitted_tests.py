#!/usr/bin/env python3
"""Authenticate/snapshot whole images,then execute actual emitted DATA predicates."""
import ast
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
images=[]
for arg,builder in zip(sys.argv[1:],('build_qwen38_empty_receipt_poll_root.py','build_qwen38_flat_matrix_grammar_root.py')):
    directory=pathlib.Path(arg);assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    tree=ast.parse((here.parent/builder).read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert len(pins)==5
    image={}
    for name,size,digest in pins:
        path=directory/name;assert not path.is_symlink()
        with path.open('rb') as source:
            before=os.fstat(source.fileno());assert stat.S_ISREG(before.st_mode) and before.st_size==size
            data=source.read(size+1);after=os.fstat(source.fileno())
        identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
        assert identity(before)==identity(after) and len(data)==size and hashlib.sha256(data).hexdigest()==digest
        image[name]=data
    images.append(image)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
with tempfile.TemporaryDirectory(prefix='pioneer-flat-grammar-emitted-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);directories=[];mappings=[]
    for label,image in zip(('old','new'),images):
        directory=out/label;directory.mkdir();directories.append(directory)
        for name,data in image.items():
            with (directory/name).open('xb') as target:target.write(data)
        # Tool and processor consume the SAME authenticated snapshots,never
        # a second unchecked open of the original caller's ELF or sections.
        table=subprocess.check_output([tool+'objdump','-tC',str(directory/'program.elf')],text=True)
        entries=[]
        for name in ('BootShape','JobShape'):
            matches=[]
            for line in table.splitlines():
                fields=line.split(None,5)
                if len(fields)==6 and fields[2]=='F' and fields[5].startswith('PioneerQ38PushedWorkerLayout::'+name+'('):
                    assert 0x40000<=int(fields[0],16)<0x60000 and int(fields[4],16)>0
                    matches.append(fields[0])
            assert len(matches)==1,(name,matches);entries+=matches
        mapping=out/(label+'.entries');mapping.write_text(' '.join(entries)+'\n');mappings.append(mapping)
    binary=out/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_flat_matrix_emitted_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*map(str,directories),*map(str,mappings)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
