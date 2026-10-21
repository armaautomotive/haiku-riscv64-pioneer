#!/usr/bin/env python3
"""Print apply_patch addition for the fully offline-tested fixed cache peer."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
pins = {'text':'366e25c3ac97ad75fa9dc1e2295a71cc239cbe8d0e12a8452d14574082745a9e',
        'affine':'189a185d35afe0aa22f4129adc708debada7a601736a09ffb8c6433b381512d6',
        'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images = {}
for section, pin in pins.items():
    data = (root / (section+'.bin')).read_bytes()
    assert data and len(data)%4 == 0 and len(data)<=8192
    assert hashlib.sha256(data).hexdigest()==pin
    images[section]=data
assert len(images['affine'])<=0x1500 and struct.unpack('<I',images['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_BLOCK_AFFINE_PEER_IMAGE_H',
       '#define PIONEER_Q38_BLOCK_AFFINE_PEER_IMAGE_H','#include <stdint.h>',
       '// Separate qualification candidate,NO native/full-model speed claim.',
       '// Exact Q4 metadata cached in initialized Dst32 128..191;Work ONLY.']
for section,suffix in [('text','Word'),('affine','PrefixWord')]:
    data=images[section]; argument='word' if section=='text' else 'address'
    lines+=['// '+section+' SHA256 '+pins[section],
            'static inline uint32_t PioneerQ38BlockAffinePeer'+suffix+'(unsigned '+argument+')',
            '{',' static const uint32_t code[]={']
    words=struct.unpack('<'+str(len(data)//4)+'I',data)
    for offset in range(0,len(words),8):
        lines.append('  '+','.join('0x%08xU'%w for w in words[offset:offset+8])+',')
    lines+=[' };',' static_assert(sizeof(code)=='+str(len(data))+' && sizeof(code)<='+str(8192 if section=='text' else 0x1500)+',"bounded exact block-affine peer");']
    if section=='text':
        lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
    else:
        lines+=[' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
                ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;']
    lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38BlockAffinePeerImage.h')
for line in lines: print('+'+line)
print('*** End Patch')
