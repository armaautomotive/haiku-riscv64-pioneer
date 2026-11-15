#!/usr/bin/env python3
"""Emit immutable full-card Q4 DATA candidates with the complete ROOT spill."""
import hashlib
import os
import pathlib
import re
import sys
from generate_qwen38_token_programs import authenticated,pins
from PioneerQ38SpillQ4ArtifactPins import ROOT_PINS,WORKER_PINS,SFPNOP_WORKER_PINS

args=sys.argv[1:];sfpnop='--sfpnop' in args
if sfpnop:
 assert args.count('--sfpnop')==1;args.remove('--sfpnop')
assert __debug__ and len(args)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker,output=map(pathlib.Path,args)
assert all(p.is_absolute() for p in (root,worker,output)) and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
roots=authenticated(root,ROOT_PINS)
worker_pins=SFPNOP_WORKER_PINS if sfpnop else WORKER_PINS
peers=authenticated(worker,worker_pins)
source=(here/'PioneerQ38FullOffloadMacroQ4Programs.h').read_bytes()
assert hashlib.sha256(source).hexdigest()=='da41eb6094faf1d8061ddd297ec399beb209fd6dcf1e903860f4d7421fd46767'
text=source.decode('ascii')
def array(label,data):
 assert data and len(data)%4==0
 digest=hashlib.sha256(data).hexdigest()
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 value='// SHA256 '+digest+'\nstatic const uint32_t '+label+'[]={\n'
 value+='\n'.join(' '+','.join('0x%08xU'%w for w in words[i:i+8])+',' for i in range(0,len(words),8))
 return value+'\n};\nstatic_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");'
for label,name in (('RootBoot','boot.bin'),('RootText','text.bin'),('RootTerminal','terminal.bin'),('RootAux','conv.bin')):
 pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
 text,count=re.subn(pattern,lambda _:array(label,roots[name]),text,flags=re.S);assert count==1,label
for label,name in (('PeerBoot','boot.bin'),('PeerFront','globalworker.bin'),('PeerPrecise','precise.bin'),
 ('PeerDense','dense.bin'),('PeerFfn','ffn.bin'),('PeerService','service.bin'),('PeerFetch','fetch.bin'),
 ('PeerAffine','affine.bin'),('PeerText','text.bin'),('PeerGdn','gdn.bin')):
 pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
 text,count=re.subn(pattern,lambda _:array(label,peers[name]),text,flags=re.S);assert count==1,label
text=text.replace('PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_PROGRAMS_H')
text=text.replace('PioneerQ38FullOffloadMacroQ4ProgramsRole.h','PioneerQ38FullOffloadSpillQ4ProgramsRole.h')
text=text.replace('PioneerQ38FullOffloadMacroQ4Programs','PioneerQ38FullOffloadSpillQ4Programs')
text=text.replace('MacroQ4Decode=1,MacroQ4Sequences=2,MacroQ4PrivateJobConfig=1,MacroQ4InertReturn=1};',
 'MacroQ4Decode=1,MacroQ4Sequences=2,MacroQ4PrivateJobConfig=1,MacroQ4InertReturn=1,SpillRoot=1,RootSections=5};')
if sfpnop:
 text=text.replace('PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_PROGRAMS_H',
                    'PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP_PROGRAMS_H')
 text=text.replace('PioneerQ38FullOffloadSpillQ4ProgramsRole.h',
                    'PioneerQ38FullOffloadSpillQ4SfpNopProgramsRole.h')
 text=text.replace('PioneerQ38FullOffloadSpillQ4Programs',
                    'PioneerQ38FullOffloadSpillQ4SfpNopPrograms')
 text=text.replace('SpillRoot=1,RootSections=5};','SpillRoot=1,RootSections=5,SfpConfigNop=1};')
marker='using PioneerQ38TokenPushedL1::Span;'
assert text.count(marker)==1
text=text.replace(marker,array('RootSpill',roots['rootspill.bin'])+'\n'+marker)
assert text.count('worker?10:4;')==1
text=text.replace('worker?10:4;','worker?10:5;')
marker='  case 3:return {0x64000,unsigned(sizeof(RootAux)/4)};'
assert text.count(marker)==1
text=text.replace(marker,'  case 3:return {0x60008,unsigned(sizeof(RootSpill)/4)};\n'+
 '  case 4:return {0x64000,unsigned(sizeof(RootAux)/4)};')
marker=' if(target>=reinterpret_cast<uintptr_t>(RootAux) && target-reinterpret_cast<uintptr_t>(RootAux)<sizeof(RootAux))return false;'
assert text.count(marker)==1
text=text.replace(marker,marker+'\n if(target>=reinterpret_cast<uintptr_t>(RootSpill) && target-reinterpret_cast<uintptr_t>(RootSpill)<sizeof(RootSpill))return false;')
marker='   case 3:data=RootAux;break;'
assert text.count(marker)==1
text=text.replace(marker,'   case 3:data=RootSpill;break;\n   case 4:data=RootAux;break;')
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',text))==15
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t Peer[A-Za-z]+',text))==10
for label,name in (('PeerBoot','boot.bin'),('PeerFront','globalworker.bin'),('PeerPrecise','precise.bin'),
 ('PeerDense','dense.bin'),('PeerFfn','ffn.bin'),('PeerService','service.bin'),('PeerFetch','fetch.bin'),
 ('PeerAffine','affine.bin'),('PeerText','text.bin'),('PeerGdn','gdn.bin')):
 digest=hashlib.sha256(peers[name]).hexdigest()
 match=re.search(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t '+label+r'\[\]',text)
 assert match and match.group(1)==digest,label
with output.open('x',encoding='ascii') as stream:stream.write(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(),output.name,flush=True)
print('DATA_ONLY '+('tag101 SFPNOP-ordered candidate' if sfpnop else 'tag100 macro-Q4 candidate')+
 ' full Q4 peers plus five-segment ROOT;no install/run/performance result',flush=True)
