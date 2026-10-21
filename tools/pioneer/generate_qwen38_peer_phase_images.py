#!/usr/bin/env python3
"""Print apply_patch only for independently pinned offline root+peer DATA profiler."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==3
pins={
 'Root':{'text':'a829dc676a6f38e1695c8956b6137e8aa3a69fd46c6efd0a3d2c056dccd8bfb2','output':'705df90348094d9ef94e1059470c1572bed3642fabda6853685f23ece9bd404a'},
 'Peer':{'text':'37af1da2262d48f1fe61d717769f8a4450e76788660ff9a1dfcd443257c5d813','affine':'68fa5e2164715f9030cb4dd2e05e2645d53e25ec0cdfa55bc4b8eef3b82fc4e1','fetch':'439d2dae42516799a4dcc08e60ad929b52f47e1bf2bc09da7f1c7eca987c3773'}}
images={}
for role,name in zip(('Root','Peer'),sys.argv[1:]):
 root=pathlib.Path(name)
 boot=(root/'boot.bin').read_bytes()
 assert hashlib.sha256(boot).hexdigest()=='e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b' and struct.unpack('<I',boot)[0]==0x0006c06f
 for section,pin in pins[role].items():
  data=(root/(section+'.bin')).read_bytes()
  limit=0x1500 if section=='affine' else 0x700 if section=='fetch' else 8192
  assert data and len(data)%4==0 and len(data)<=limit and hashlib.sha256(data).hexdigest()==pin
  images[role,section]=data
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_PEER_PHASE_IMAGES_H','#define PIONEER_Q38_PEER_PHASE_IMAGES_H','#include <stdint.h>',
 '// Pinned private timing DATA profiler. Frozen bytes ONLY;NO source/IO/ownership/native authority.']
for (role,section),data in images.items():
 name='PioneerQ38PeerPhase'+role+section.capitalize()
 lines+=['// SHA256 '+pins[role][section],'static inline uint32_t '+name+'(unsigned word)','{',' static const uint32_t code[]={']
 words=struct.unpack('<'+'I'*(len(data)//4),data)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(data))+',"exact bounded timing image");',
  ' return word<sizeof(code)/4?code[word]:0;','}']
for role in ('Root','Peer'):
 prefix='PioneerQ38PeerPhase'+role
 lines+=['static inline uint32_t '+prefix+'Word(unsigned word){return word<2048?'+prefix+'Text(word):UINT32_MAX;}',
  'static inline uint32_t '+prefix+'PrefixWord(unsigned address){',
  ' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;']
 if role=='Root':lines+=[' return '+prefix+'Output((address-0x70000)/4);']
 else:lines+=[' if(address<0x70700)return '+prefix+'Fetch((address-0x70000)/4);',
  ' return address>=0x70b00?'+prefix+'Affine((address-0x70b00)/4):0;']
 lines+=['}']
lines+=['#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38PeerPhaseImages.h')
for line in lines:print('+'+line)
print('*** End Patch')
