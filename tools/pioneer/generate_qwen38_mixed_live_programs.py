#!/usr/bin/env python3
"""Mechanically emit authenticated immutable code DATA, not native authority."""
import ast
import hashlib
import os
import pathlib
import sys
assert len(sys.argv)==4 and not any(k.startswith('PIONEER_') for k in os.environ)
root,worker,out=map(pathlib.Path,sys.argv[1:])
assert root.is_absolute() and worker.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent
def pins(script):
 tree=ast.parse((here/'tests'/script).read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def pin(directory,records):
 for name,size,digest in records:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,pins('run_pioneer_q38_mixed_live_block_root_tests.py'))
pin(worker,pins('run_pioneer_q38_mixed_worker_tests.py'))
units=[(root,'boot',0x6000,'RootBoot'),(root,'code',0x40000,'RootCode')]
units += [(worker,name,address,'Peer'+label) for name,address,label in (
 ('boot',0x6000,'Boot'),('mixed',0x20000,'Mixed'),('dense',0x40000,'Dense'),('ffn',0x60000,'Ffn'),
 ('service',0x68000,'Output'),('affine',0x70b00,'Affine'),('text',0x72000,'Service'),('gdn',0xb8000,'Gdn'))]
lines=['/* SPDX-License-Identifier: MIT */','// Mechanically generated from COMPLETE authenticated artifacts. Immutable DATA ONLY.',
 '// No code/source/init/clock/host/map/journal/owner or execution capability.',
 '#ifndef PIONEER_Q38_MIXED_LIVE_PROGRAMS_H','#define PIONEER_Q38_MIXED_LIVE_PROGRAMS_H','#include "PioneerQ38MixedLiveFootprint.h"']
for directory,name,address,label in units:
 data=(directory/(name+'.bin')).read_bytes();assert data and len(data)%4==0
 lines+=['// SHA256 '+hashlib.sha256(data).hexdigest(),'static const uint32_t PioneerQ38MixedLive'+label+'[]={']
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 lines+=[' '+','.join('0x'+format(w,'08x')+'U' for w in words[i:i+8])+',' for i in range(0,len(words),8)]
 lines+=['};','static_assert(sizeof(PioneerQ38MixedLive'+label+')=='+str(len(data))+',"exact complete '+label+' DATA");']
lines+=['static inline bool PioneerQ38MixedLiveProgramWord(unsigned w,uint32_t address,uint32_t& output){',
 ' if(w>=120 || (address&3))return false;',' for(unsigned section=0;section<PioneerQ38MixedLiveCodeSpans(w);section++){',
 '  const auto span=PioneerQ38MixedLiveCodeSpan(w,section);if(address<span.address || address-span.address>=span.words*4)continue;',
 '  const uint32_t* data=nullptr;','  if(!w)data=section?PioneerQ38MixedLiveRootCode:PioneerQ38MixedLiveRootBoot;',
 '  else switch(section){']
for i,(_,_,_,label) in enumerate(units[2:]):lines+=['   case '+str(i)+':data=PioneerQ38MixedLive'+label+';break;']
lines+=['  }','  if(!data)return false;output=data[(address-span.address)/4];return true;',' }',
 ' return false; // caller output untouched;NO zero-filled gaps','}','#endif','']
out.write_text('\n'.join(lines))
print(hashlib.sha256(out.read_bytes()).hexdigest()+' '+str(out)+';DATA ONLY/ALL10 complete sections/NO native or launch grant')
