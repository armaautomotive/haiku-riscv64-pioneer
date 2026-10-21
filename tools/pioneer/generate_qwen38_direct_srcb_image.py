#!/usr/bin/env python3
"""Print only an apply_patch addition for the exact offline-tested scale peer."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'209fc6b8d40ed0ac7216c16ec9657ed0d83259e2ded53b35524c8cc6507bf0ef',
      'affine':'083330550580cc8ba821456c95b256ff8ea1d9742bb12230b06af469392f856a',
      'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images={}
for section,pin in pins.items():
 data=(root/(section+'.bin')).read_bytes()
 assert data and len(data)%4==0 and len(data)<=8192 and hashlib.sha256(data).hexdigest()==pin
 images[section]=data
assert len(images['affine'])<=0x1500 and struct.unpack('<I',images['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_DIRECT_SRCB_PEER_IMAGE_H',
       '#define PIONEER_Q38_DIRECT_SRCB_PEER_IMAGE_H','#include <stdint.h>',
       '// Separate qualification image;Work direct SrcB from validated inputs with freshly initialized224B tail;exact register replay/raw-dot arithmetic.',
       '// No native/model/speed claim from offline tests.']
for section,suffix in [('text','Word'),('affine','PrefixWord')]:
 data=images[section];arg='word' if section=='text' else 'address'
 lines+=['// '+section+' SHA256 '+pins[section],
         'static inline uint32_t PioneerQ38DirectSrcBPeer'+suffix+'(unsigned '+arg+')',
         '{',' static const uint32_t code[]={']
 words=struct.unpack('<'+str(len(data)//4)+'I',data)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(data))+' && sizeof(code)<='+str(8192 if section=='text' else 0x1500)+',"bounded exact direct-srcb peer");']
 if section=='text':lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
 else:lines+=[' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
              ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;']
 lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38DirectSrcBPeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
