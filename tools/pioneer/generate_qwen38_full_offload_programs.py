#!/usr/bin/env python3
"""Generate model-code DATA only from the exact full-offload ROOT and old peers."""
import hashlib
import os
import pathlib
import re
import sys

assert __debug__ and len(sys.argv)==3
directory,output=map(pathlib.Path,sys.argv[1:])
assert directory.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
source=(here/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(source).hexdigest()=='692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'
text=source.decode()
pins=(('RootBoot','boot',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
      ('RootText','text',130964,'f00c6833db360f9a2b9dfb1471cbe98ecbfc83dd96418057d4ba52967f662a49'),
      ('RootTerminal','terminal',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
      ('RootAux','conv',32708,'a12c858b03268d39923365c6280fe320a9708dc2b3a651c8ca07f0ef0e02cc94'))
for label,name,size,digest in pins:
    with (directory/(name+'.bin')).open('rb') as file:
        before=os.fstat(file.fileno());data=file.read(size+1);after=os.fstat(file.fileno())
    identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
    assert identity(before)==identity(after) and len(data)==size and hashlib.sha256(data).hexdigest()==digest
    words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
    array='// SHA256 '+digest+'\nstatic const uint32_t '+label+'[]={\n'
    array+='\n'.join(' '+','.join('0x%08xU'%w for w in words[i:i+8])+',' for i in range(0,len(words),8))
    array+='\n};\nstatic_assert(sizeof('+label+')=='+str(size)+',"exact complete '+label+' DATA");'
    pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
    text,count=re.subn(pattern,lambda _:array,text,flags=re.S);assert count==1
text=text.replace('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H')
text=text.replace('PioneerQ38TokenPushedPrograms','PioneerQ38FullOffloadPrograms')
assert text.count('Family=0x33505451,Version=1,CancelBoundary=1')==1
text=text.replace('Family=0x33505451,Version=1,CancelBoundary=1','Family=0x33505451,Version=1,CancelBoundary=1,EogCount=5')
text=text.replace('// Mechanically generated from fixed experimental warm-admission ROOT/QGW3 artifacts.',
                  '// Generated full-offload all5-EOG ROOT plus byte-exact existing G3 peers.')
text=text.replace('// Full-model/native journal/restore/Work/physical/TPS qualification remains separate.',
                  '// Build DATA only;physical/numerical/Assistant release qualification remains separate.')
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',text))==13
output.write_text(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(),output.name)
print('PASS generated all13 code arrays;four pinned ROOT sections/nine unchanged peer sections;NO learned weights or execution')
