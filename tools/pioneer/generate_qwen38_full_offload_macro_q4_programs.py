#!/usr/bin/env python3
"""Executable-code DATA only:unchanged5-EOG ROOT+complete macro-Q4 peers."""
import hashlib
import os
import pathlib
import re
import sys
from generate_qwen38_token_programs import authenticated,pins

assert __debug__ and len(sys.argv)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
directory,output=map(pathlib.Path,sys.argv[1:])
assert directory.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
peer=authenticated(directory,pins(here/'build_qwen38_macro_q4_decode_worker.py','PINS'))
source=(here/'PioneerQ38FullOffloadVectorQ4Programs.h').read_bytes()
assert hashlib.sha256(source).hexdigest()=='3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
text=source.decode('ascii')
mapping={'PeerBoot':'boot','PeerFront':'globalworker','PeerPrecise':'precise','PeerDense':'dense',
         'PeerFfn':'ffn','PeerService':'service','PeerFetch':'fetch','PeerAffine':'affine','PeerText':'text','PeerGdn':'gdn'}
for label,name in mapping.items():
 data=peer[name+'.bin'];digest=hashlib.sha256(data).hexdigest()
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 array='// SHA256 '+digest+'\nstatic const uint32_t '+label+'[]={\n'
 array+='\n'.join(' '+','.join('0x%08xU'%w for w in words[i:i+8])+',' for i in range(0,len(words),8))
 array+='\n};\nstatic_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");'
 pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
 text,count=re.subn(pattern,lambda _:array,text,flags=re.S);assert count==1
text=text.replace('PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4_PROGRAMS_H')
text=text.replace('PioneerQ38FullOffloadVectorQ4Programs','PioneerQ38FullOffloadMacroQ4Programs')
marker='enum {VectorQ4Metadata=1,MetadataRawUnpacks=1,MetadataScalarProducts=0};'
assert text.count(marker)==1
text=text.replace(marker,marker+'\nenum {MacroQ4Decode=1,MacroQ4Sequences=2,MacroQ4PrivateJobConfig=1,MacroQ4InertReturn=1};')
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',text))==14
with output.open('x',encoding='ascii') as stream:stream.write(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(),output.name,flush=True)
print('DATA_ONLY14 complete code sections/old4 ROOT unchanged/ten authenticated macro-Q4 peers;'
      'NO learned weights,installation,execution or speed authority',flush=True)
