#!/usr/bin/env python3
"""Print ONLY apply_patch addition for the separately tested raw-input peer."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'6f7ceb71adf49d35330a3e02cc2a5678289272b27ddaab1e3dc160c8dc4dd498',
 'affine':'1568cfc02a808a218551d0683642429f203217f5a21254d857c4d846a1d82b35',
 'fetch':'0986412151b4c104cf02483d6c6b5a88b2887167beda1e88bf1c31b0e1b5c86b',
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
 lines.extend([' };',' static_assert(sizeof('+name+')=='+str(len(b))+',"exact independently pinned direct-batch input peer");'])
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_BATCH_RAW_PEER_IMAGE_H',
 '#define PIONEER_Q38_BATCH_RAW_PEER_IMAGE_H','#include <stdint.h>',
 '// Separate exact-tag20 qualification ONLY,not native/model/speed permission.']
for section,pin in pins.items():lines.append('// '+section+' SHA256 '+pin)
lines+=['static inline uint32_t PioneerQ38BatchRawPeerWord(unsigned word)','{']
array(lines,data['text'],'code')
lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;','}',
 'static inline uint32_t PioneerQ38BatchRawPeerPrefixWord(unsigned address)','{']
array(lines,data['fetch'],'fetch');array(lines,data['affine'],'affine')
lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
 ' if(address-0x70000<sizeof(fetch))return fetch[(address-0x70000)/4];',
 ' return address>=0x70b00 && address-0x70b00<sizeof(affine)?affine[(address-0x70b00)/4]:0;',
 '}','#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38BatchRawPeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
