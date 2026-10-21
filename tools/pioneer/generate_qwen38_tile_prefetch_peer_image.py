#!/usr/bin/env python3
"""Print ONLY apply_patch addition for the independently tested tile_prefetch peer."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'4e3730b2b8594add7c4b2f35d90e46baf7d324c047276f01b235fa5bd72906b4',
 'affine':'61bf0a29575a4de077c4f985e46655d84b299ed1b12d804b550187bfdf7816e2',
 'fetch':'6006161e446a9e7afd2c75bc529cf7c20f983f6f85110f8376bb9147a1327009',
 'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
data={}
for section,pin in pins.items():
 b=(root/(section+'.bin')).read_bytes()
 assert b and len(b)%4==0 and len(b)<=8192 and hashlib.sha256(b).hexdigest()==pin
 data[section]=b
assert len(data['fetch'])<=0x700 and len(data['affine'])<=0x1500 and struct.unpack('<I',data['boot'])[0]==0x0006c06f
def array(lines,b,name):
 words=struct.unpack('<'+'I'*(len(b)//4),b);lines.append(' static const uint32_t '+name+'[]={')
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines.extend([' };',' static_assert(sizeof('+name+')=='+str(len(b))+',"exact independently pinned tile_prefetch-barrier peer");'])
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_TILE_PREFETCH_PEER_IMAGE_H',
 '#define PIONEER_Q38_TILE_PREFETCH_PEER_IMAGE_H','#include <stdint.h>',
 '// Separate exact-tag23 qualification ONLY,not native/model/speed permission.']
for section,pin in pins.items():lines.append('// '+section+' SHA256 '+pin)
lines+=['static inline uint32_t PioneerQ38TilePrefetchPeerWord(unsigned word)','{'];array(lines,data['text'],'code')
lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;','}',
 'static inline uint32_t PioneerQ38TilePrefetchPeerPrefixWord(unsigned address)','{']
array(lines,data['fetch'],'fetch');array(lines,data['affine'],'affine')
lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
 ' if(address-0x70000<sizeof(fetch))return fetch[(address-0x70000)/4];',
 ' return address>=0x70b00 && address-0x70b00<sizeof(affine)?affine[(address-0x70b00)/4]:0;','}','#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38TilePrefetchPeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
