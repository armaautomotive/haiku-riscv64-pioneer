#!/usr/bin/env python3
"""Print ONE apply_patch for complete pinned QOP2/QOI2 code DATA only.
Validate both boots and all six images before ANY output. No file writes,
SDK access, native registry, device mappings or launch authority.
"""
import hashlib
import pathlib
import struct
import sys

PINS = {
 'root-main': ('text', 6196, '7d93e46d85c1077a7509d81df26484988e129b7588a50b7b232b2152dcda64ea', 'RootMain'),
 'root-prefix': ('prefix', 8284, 'e6961eec79ed035b55b985271a4631a314b32eba91f4a36708843d9d9d004e91', 'RootPrefix'),
 'root-graph': ('graph', 15128, '064a9c89da0e2b5e9c2a55fceafee48c862834008ea526ec28e02b1880a5d718', 'RootGraph'),
 'peer-main': ('text', 8132, 'b5460d35a05aa727273768e3926a48405dccedfda3bb918156ad1869e1f52348', 'PeerMain'),
 'peer-affine': ('affine', 5248, 'e2fe73ecbe7b78e4aca9b069ee70ead09d0d7f61575fdab83ce2e76a1879a785', 'PeerAffine'),
 'peer-helper': ('ffn', 6200, '6892cd5d863708127b9badacf38f6d3ea4135493c85da090eabbc8ccf1803184', 'PeerHelper'),
}

def main(args):
 if len(args)!=3 or args[2] not in PINS:
  raise ValueError('expected ROOT PEER and one exact QOP2/QOI2 section')
 images={}
 for kind,path in [('root',pathlib.Path(args[0])),('peer',pathlib.Path(args[1]))]:
  if (path/'boot.bin').read_bytes()!=struct.pack('<I',0x0006c06f):
   raise ValueError('exact shared boot mismatch')
  for name,(section,size,pin,_) in PINS.items():
   if not name.startswith(kind+'-'):continue
   data=(path/(section+'.bin')).read_bytes()
   if len(data)!=size or len(data)%4 or hashlib.sha256(data).hexdigest()!=pin:
    raise ValueError('complete pinned QOP2/QOI2 section mismatch: '+name)
   images[name]=data
 name=args[2];_,size,pin,suffix=PINS[name]
 guard='PIONEER_Q38_NEXT_GDN_OUTPUT_'+suffix.upper()+'_IMAGE_H';symbol='PioneerQ38NextGdnOutput'+suffix
 lines=['/* SPDX-License-Identifier: MIT */','#ifndef '+guard,'#define '+guard,'#include <stdint.h>',
  '// Frozen complete QOP2/QOI2 DATA,not native/source/producer/owner authority.','// SHA256 '+pin,
  'static const uint32_t '+symbol+'[]={']
 words=struct.unpack('<'+str(size//4)+'I',images[name])
 for at in range(0,len(words),8):lines.append(' '+','.join('0x%08xU'%v for v in words[at:at+8])+',')
 lines+=['};','static_assert(sizeof('+symbol+')=='+str(size)+',"exact complete QOP2/QOI2 section");','#endif']
 print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+symbol+'.h')
 for line in lines:print('+'+line)
 print('*** End Patch')

if __name__=='__main__':main(sys.argv[1:])
