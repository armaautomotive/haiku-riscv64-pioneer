#!/usr/bin/env python3
"""Print apply_patch ONLY for independently pinned OFFLINE local Q4 decoder."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'69d8d87eb0420c988c3c9a74f04ef8591a4ec41eca442a1a45dcc17f00d0a90e',
      'affine':'f0a21f6565ab2f19ccfa86b4c8546e47278eb0687d2dfc53cfcac2558f186200',
      'fetch':'afa2183a5858b092b8fdbf87aaa352505f136158e43b3de460c29698f630508c',
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
 lines.extend([' };',' static_assert(sizeof('+name+')=='+str(len(b))+',"exact independently pinned local Q4 decoder");'])
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_LOCAL_Q4_PEER_IMAGE_H',
       '#define PIONEER_Q38_LOCAL_Q4_PEER_IMAGE_H','#include <stdint.h>',
       '// OFFLINE image-registry proof ONLY,not native/model/Assistant/speed permission.']
for section,pin in pins.items():lines.append('// '+section+' SHA256 '+pin)
lines+=['static inline uint32_t PioneerQ38LocalQ4PeerWord(unsigned word)','{'];array(lines,data['text'],'code')
lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;','}',
        'static inline uint32_t PioneerQ38LocalQ4PeerPrefixWord(unsigned address)','{']
array(lines,data['fetch'],'decoder');array(lines,data['affine'],'affine')
lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
        ' if(address-0x70000<sizeof(decoder))return decoder[(address-0x70000)/4];',
        ' return address>=0x70b00 && address-0x70b00<sizeof(affine)?affine[(address-0x70b00)/4]:0;','}','#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38LocalQ4PeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
