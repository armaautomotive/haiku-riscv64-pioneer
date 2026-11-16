#!/usr/bin/env python3
"""Create a distinct tag104 program package by replacing only pinned Q4 peer images."""
import ast
import hashlib
import pathlib
import re
import sys

if not __debug__ or len(sys.argv) != 3:
    raise SystemExit('usage: generate_qwen38_prepared_byte_lut_full_offload_programs.py BYTE_LUT_WORKER OUT_HEADER')
here=pathlib.Path(__file__).resolve().parent
worker,output=map(pathlib.Path,sys.argv[1:])
base=here/'PioneerQ38FullOffloadPreparedLutQ4Programs.h'
assert worker.is_absolute() and worker.is_dir() and not worker.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
base_bytes=base.read_bytes()
assert hashlib.sha256(base_bytes).hexdigest()=='39fbf5204e4301d4fb6df2f89767989f893d57e256e89b56bb24a77e9314dfb7'
builder=ast.parse((here/'build_qwen38_prepared_exact_q4_worker.py').read_text())
records=next(ast.literal_eval(n.value) for n in builder.body if isinstance(n,ast.Assign)
             and any(isinstance(t,ast.Name) and t.id=='BYTE_LUT_PINS' for t in n.targets))
rom={}
for name,size,digest in records:
    data=(worker/name).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
    rom[name]=data
labels={'PeerBoot':'boot','PeerFront':'globalworker','PeerPrecise':'precise','PeerDense':'dense',
        'PeerFfn':'ffn','PeerService':'service','PeerFetch':'fetch','PeerAffine':'affine',
        'PeerText':'text','PeerGdn':'gdn'}
text=base_bytes.decode('ascii')
for label,section in labels.items():
    data=rom[section+'.bin'];words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
    body='// SHA256 '+hashlib.sha256(data).hexdigest()+'\nstatic const uint32_t '+label+'[]={\n'
    body+='\n'.join(' '+','.join('0x%08xU'%w for w in words[i:i+8])+',' for i in range(0,len(words),8))
    body+='\n};\nstatic_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");'
    pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
    text,count=re.subn(pattern,lambda _:body,text,flags=re.S)
    assert count==1,label
text=text.replace('PioneerQ38FullOffloadPreparedLutQ4ProgramsRole.h','PioneerQ38FullOffloadPreparedByteLutQ4ProgramsRole.h')
text=text.replace('PIONEER_Q38_FULL_OFFLOAD_PREPARED_LUT_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_PREPARED_BYTE_LUT_Q4_PROGRAMS_H')
text=text.replace('PioneerQ38FullOffloadPreparedLutQ4Programs','PioneerQ38FullOffloadPreparedByteLutQ4Programs')
text=text.replace('PreparedActivationLutWorker=1','PreparedActivationByteLutWorker=1')
text=text.replace('exact prepared-Q4 ROOT+activation-LUT worker candidate','exact prepared-Q4 ROOT+byte-index activation-LUT worker candidate')
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',text))==15
output.write_text(text,encoding='ascii')
print(hashlib.sha256(output.read_bytes()).hexdigest(),output.name,flush=True)
print('BYTE-LUT exact ROOT sections retained;ten worker sections replaced from whole pinned ROMs;no execution/device authority',flush=True)
