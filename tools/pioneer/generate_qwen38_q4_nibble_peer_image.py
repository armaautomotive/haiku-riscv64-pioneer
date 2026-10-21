#!/usr/bin/env python3
"""Print apply_patch ONLY for the independently pinned OFFLINE Q4 decoder."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'8fd12aab6195800e93a64719ce56fce294adc8d3ff67fedebc4d7ef31c45289a',
      'affine':'c7cefc9ec3a8ad14bfb3f6b667ca41178bcfbbb2ae9df47abfbe9df8e4128145',
      'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
data={}
for section,pin in pins.items():
 b=(root/(section+'.bin')).read_bytes()
 assert b and len(b)%4==0 and len(b)<=8192 and hashlib.sha256(b).hexdigest()==pin
 data[section]=b
assert len(data['affine'])<=0x1500 and struct.unpack('<I',data['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_Q4_NIBBLE_PEER_IMAGE_H',
       '#define PIONEER_Q38_Q4_NIBBLE_PEER_IMAGE_H','#include <stdint.h>',
       '// Pinned peer for separate exact-tag26 qualification;NO model/Assistant/speed promotion.']
for section,suffix in [('text','Word'),('affine','PrefixWord')]:
 b=data[section];arg='word' if section=='text' else 'address'
 lines+=['// '+section+' SHA256 '+pins[section],
         'static inline uint32_t PioneerQ38Q4NibblePeer'+suffix+'(unsigned '+arg+')','{',
         ' static const uint32_t code[]={']
 words=struct.unpack('<'+'I'*(len(b)//4),b)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(b))+' && sizeof(code)<=8192,"exact pinned offline Q4 nibble peer");']
 if section=='text':lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;']
 else:lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
              ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;']
 lines+=['}']
lines+=['#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38Q4NibblePeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
