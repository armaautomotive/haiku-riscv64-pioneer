#!/usr/bin/env python3
"""Print apply_patch ONLY for the independently pinned offline root."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==2
root=pathlib.Path(sys.argv[1])
pins={'text':'56623f8942b7a9a018f557e7ce157624cb031e19f1780d32622760cd90327d62',
 'output':'06e40509f3570a28a9aedb08afade69cc0850084c37103f790f49a362214b165',
 'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images={}
for section,pin in pins.items():
 b=(root/(section+'.bin')).read_bytes()
 assert b and len(b)%4==0 and len(b)<=8192 and hashlib.sha256(b).hexdigest()==pin
 images[section]=b
assert struct.unpack('<I',images['boot'])[0]==0x0006c06f
lines=['/* SPDX-License-Identifier: MIT */','#ifndef PIONEER_Q38_COMMAND_REUSE_ROOT_IMAGE_H',
 '#define PIONEER_Q38_COMMAND_REUSE_ROOT_IMAGE_H','#include <stdint.h>',
 '// Separate exact-tag24 qualification ONLY;not native/speed/Assistant permission.']
for section,suffix in [('text','Word'),('output','PrefixWord')]:
 b=images[section];arg='word' if section=='text' else 'address'
 lines+=['// '+section+' SHA256 '+pins[section],
  'static inline uint32_t PioneerQ38CommandReuseRoot'+suffix+'(unsigned '+arg+')','{',' static const uint32_t code[]={']
 words=struct.unpack('<'+'I'*(len(b)//4),b)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof(code)=='+str(len(b))+' && sizeof(code)<=8192,"exact bounded command-reuse root");']
 if section=='text':lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;']
 else:lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
  ' return address-0x70000<sizeof(code)?code[(address-0x70000)/4]:0;']
 lines+=['}']
lines+=['#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38CommandReuseRootImage.h')
for line in lines:print('+'+line)
print('*** End Patch')
