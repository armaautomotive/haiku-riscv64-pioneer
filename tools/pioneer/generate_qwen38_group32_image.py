#!/usr/bin/env python3
"""Print only an apply_patch addition for the exact offline-tested group32 peer."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'6f0087f798dfb657eb137acad08e65541e121747403c2d04e42c2661fdd14948',
 'affine':'2cb765229203947400a2cc3710443cfd6ce572d5129829e61fed08e4a8109097',
 'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images={}
for section,pin in pins.items():
 data=(root/(section+'.bin')).read_bytes()
 assert data and len(data)%4==0 and len(data)<=8192 and hashlib.sha256(data).hexdigest()==pin
 images[section]=data
assert len(images['affine'])<=0x1500 and struct.unpack('<I',images['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_GROUP32_PEER_IMAGE_H',
 '#define PIONEER_Q38_GROUP32_PEER_IMAGE_H','#include <stdint.h>',
 '// Separate qualification artifact:Work-Q4 group32 intentionally changes FP32 rounding.',
 '// No native/model-quality/speed claim from offline tests.']
for section,suffix in [('text','Word'),('affine','PrefixWord')]:
 data=images[section];arg='word' if section=='text' else 'address'
 lines+=['// '+section+' SHA256 '+pins[section],
  'static inline uint32_t PioneerQ38Group32Peer'+suffix+'(unsigned '+arg+')','{',' static const uint32_t code[]={']
 words=struct.unpack('<'+str(len(data)//4)+'I',data)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(data))+' && sizeof(code)<='+str(8192 if section=='text' else 0x1500)+',"bounded exact group32 peer");']
 if section=='text':lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
 else:lines+=[' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
  ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;']
 lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38Group32PeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
