#!/usr/bin/env python3
"""Print only an apply_patch addition for the exact offline-tested scale peer."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'a3af0d566fa867d1a75b8612bc3c57799422f4e709c7826218befb52aafe107a',
      'affine':'06dce3acf1db0a00b16a6fcf2230eded0e4aa4ab11933997af0004f519872db1',
      'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images={}
for section,pin in pins.items():
 data=(root/(section+'.bin')).read_bytes()
 assert data and len(data)%4==0 and len(data)<=8192 and hashlib.sha256(data).hexdigest()==pin
 images[section]=data
assert len(images['affine'])<=0x1500 and struct.unpack('<I',images['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_AFFINE_RAWDOT_PEER_IMAGE_H',
       '#define PIONEER_Q38_AFFINE_RAWDOT_PEER_IMAGE_H','#include <stdint.h>',
       '// Separate qualification image;Work register replay plus separately proved +0-accumulator dot-zero elision.',
       '// No native/model/speed claim from offline tests.']
for section,suffix in [('text','Word'),('affine','PrefixWord')]:
 data=images[section];arg='word' if section=='text' else 'address'
 lines+=['// '+section+' SHA256 '+pins[section],
         'static inline uint32_t PioneerQ38AffineRawDotPeer'+suffix+'(unsigned '+arg+')',
         '{',' static const uint32_t code[]={']
 words=struct.unpack('<'+str(len(data)//4)+'I',data)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(data))+' && sizeof(code)<='+str(8192 if section=='text' else 0x1500)+',"bounded exact affine-rawdot peer");']
 if section=='text':lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
 else:lines+=[' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
              ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;']
 lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38AffineRawDotPeerImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
