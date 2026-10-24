#!/usr/bin/env python3
"""Print ONE apply_patch for complete pinned QOP1/op6 code DATA only.
Validate both boots and all six images before ANY output. No file writes,
SDK access, native registry, device mappings or launch authority.
"""
import hashlib
import pathlib
import struct
import sys

PINS = {
 'root-main': ('text', 6196, '2cca19748a0771f2ff6e5e08ea5c704ae43cdcb83771d0df94e104734c8e84b1', 'RootMain'),
 'root-prefix': ('prefix', 8268, '5332aeac39d34773812477bfa243402c1d3fbf4b372a73f0d8a16b2718009961', 'RootPrefix'),
 'root-graph': ('graph', 14984, 'dc89a3f1e25ec58a5bf2e58d3dd84aa85e19546993f5cdda2ac3a7e4ce4592d2', 'RootGraph'),
 'peer-main': ('text', 8132, 'b5460d35a05aa727273768e3926a48405dccedfda3bb918156ad1869e1f52348', 'PeerMain'),
 'peer-affine': ('affine', 5248, 'e2fe73ecbe7b78e4aca9b069ee70ead09d0d7f61575fdab83ce2e76a1879a785', 'PeerAffine'),
 'peer-helper': ('ffn', 6156, 'b0af8b847886d04b25a5d0509b89f4db27c77619be12aefe4152660108ae7074', 'PeerHelper'),
}

def main(args):
 if len(args)!=3 or args[2] not in PINS:
  raise ValueError('expected ROOT PEER and one exact QOP1/op6 section')
 images={}
 for kind,path in [('root',pathlib.Path(args[0])),('peer',pathlib.Path(args[1]))]:
  if (path/'boot.bin').read_bytes()!=struct.pack('<I',0x0006c06f):
   raise ValueError('exact shared boot mismatch')
  for name,(section,size,pin,_) in PINS.items():
   if not name.startswith(kind+'-'):continue
   data=(path/(section+'.bin')).read_bytes()
   if len(data)!=size or len(data)%4 or hashlib.sha256(data).hexdigest()!=pin:
    raise ValueError('complete pinned QOP1/op6 section mismatch: '+name)
   images[name]=data
 name=args[2];_,size,pin,suffix=PINS[name]
 guard='PIONEER_Q38_GDN_OUTPUT_'+suffix.upper()+'_IMAGE_H';symbol='PioneerQ38GdnOutput'+suffix
 lines=['/* SPDX-License-Identifier: MIT */','#ifndef '+guard,'#define '+guard,'#include <stdint.h>',
  '// Frozen complete QOP1/op6 DATA,not native/source/producer/owner authority.','// SHA256 '+pin,
  'static const uint32_t '+symbol+'[]={']
 words=struct.unpack('<'+str(size//4)+'I',images[name])
 for at in range(0,len(words),8):lines.append(' '+','.join('0x%08xU'%v for v in words[at:at+8])+',')
 lines+=['};','static_assert(sizeof('+symbol+')=='+str(size)+',"exact complete QOP1/op6 section");','#endif']
 print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+symbol+'.h')
 for line in lines:print('+'+line)
 print('*** End Patch')

if __name__=='__main__':main(sys.argv[1:])
