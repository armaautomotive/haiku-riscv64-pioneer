#!/usr/bin/env python3
"""Authenticated HOST symbol metadata only;no ROM/processor/device input API."""
import ast
import hashlib
import json
import pathlib
import subprocess

def build_profile_map(root,output):
    here=pathlib.Path(__file__).resolve().parent
    assert root.is_absolute() and root.is_dir() and output.is_absolute() and output.is_dir()
    tree=ast.parse((here.parent/'build_qwen38_token_fault_marker_image.py').read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    elf=None
    for name,size,digest in pins:
        data=(root/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(root,name)
        if name=='program.elf':elf=data
    assert elf is not None
    snapshot=output/'root-profile-PINNED.elf';mapping=output/'root-profile.map';aliases=output/'root-profile-aliases.json'
    assert all(not p.exists() and not p.is_symlink() for p in (snapshot,mapping,aliases))
    # nm consumes the SAME already authenticated byte snapshot,not a second
    # unchecked open of the original pathname. Private output must stay exact.
    with snapshot.open('xb') as target:target.write(elf)
    nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    table=subprocess.check_output([nm,'-S','-n','--demangle',str(snapshot)],text=True)
    assert snapshot.read_bytes()==elf
    intervals={}
    for line in table.splitlines():
        row=line.split(None,3)
        if len(row)!=4 or row[2] not in ('T','t','W','w'):continue
        begin,size=int(row[0],16),int(row[1],16)
        if not size:continue
        assert not (begin|size)&3
        assert (0x40000<=begin and begin+size<=0x60000) or (0x64000<=begin and begin+size<=0x6c000) \
            or (begin==0x6000 and size==4) or (0x60000<=begin and begin+size<=0x60008)
        names=intervals.setdefault((begin,size),[])
        if row[3] not in names:names.append(row[3])
    ordered=sorted(intervals.items());last=0
    for (begin,size),names in ordered:assert begin>=last and names;last=begin+size
    assert ordered
    # Preserve the legacy first-name format for the existing histogram's
    # range IDs. ALL aliases are retained separately:shared code cannot be
    # attributed exclusively to whichever function nm happened to name first.
    payload=''.join(f'{begin:x} {size:x} {names[0]}\n' for (begin,size),names in ordered)
    with mapping.open('x') as target:target.write(payload)
    with aliases.open('x') as target:
        json.dump([dict(begin=begin,bytes=size,names=names) for (begin,size),names in ordered],target,indent=2)
        target.write('\n')
    return dict(path=mapping,elf_sha256=hashlib.sha256(elf).hexdigest(),
                map_sha256=hashlib.sha256(payload.encode()).hexdigest(),ranges=len(ordered),aliases=aliases)
