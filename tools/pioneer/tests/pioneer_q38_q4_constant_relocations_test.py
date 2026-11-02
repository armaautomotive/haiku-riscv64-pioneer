#!/usr/bin/env python3
"""Actual old/new reference audit plus independent non-text corruption controls."""
import contextlib
import io
import pathlib
import shutil
import struct
import sys
import tempfile
from pioneer_q38_q4_constant_relocations import audit, BASES

assert len(sys.argv)==3
control,image=map(pathlib.Path,sys.argv[1:])
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
with contextlib.redirect_stdout(io.StringIO()):
    assert audit(control,image,tool)==18
with tempfile.TemporaryDirectory(prefix='pioneer-q4-relocation-faults-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp)
    for name in (*(s+'.bin' for s in BASES),'program.elf'):
        shutil.copyfile(image/name,root/name)
    refusals=0
    def reject(section,offset,mask):
        path=root/(section+'.bin');original=path.read_bytes();changed=bytearray(original)
        word=struct.unpack_from('<I',changed,offset)[0]
        struct.pack_into('<I',changed,offset,word^mask)
        path.write_bytes(changed)
        refused=False
        try:
            with contextlib.redirect_stdout(io.StringIO()):audit(control,root,tool)
        except (AssertionError,KeyError):
            refused=True
        finally:
            path.write_bytes(original)
        assert refused,(section,offset,mask)
    # All otherwise unchanged sections, and non-reference fetch/affine words.
    for section in BASES:
        if section=='text':continue
        reject(section,0,1);refusals+=1
    # Change actual entry/call/constant reference instruction fields, not
    # only arbitrary code padding. Immediate +4 cannot bind the same symbol.
    for section,address,mask in (
        ('globalworker',0x21e2c,0x00400000),
        ('fetch',0x701d8,0x00001000),('fetch',0x70428,0x00400000),
        ('affine',0x71148,0x00400000),('affine',0x71190,0x00400000),
        ('affine',0x70b6c,0x00400000),('affine',0x70b74,0x00400000),
        ('affine',0x717c0,0x00400000)):
        reject(section,address-BASES[section],mask);refusals+=1
    # The moved complete helper/table bytes themselves MUST remain equal.
    for address in (0x72494,0x724e4,0x7386c,0x73888,0x738ec):
        reject('text',address-BASES['text'],1);refusals+=1
    print('PASS_CONSTANT_Q4_REFERENCE_AUDIT positive18 references corruptionRefusals'+str(refusals)+
          ';not a whole-code/numerical/native qualification')
