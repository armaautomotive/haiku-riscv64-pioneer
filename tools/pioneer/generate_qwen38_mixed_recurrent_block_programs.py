#!/usr/bin/env python3
"""Authenticate COMPLETE QRB1/QMW1 artifacts and mechanically emit DATA only."""
import ast
import hashlib
import os
import pathlib
import sys
if len(sys.argv)!=4 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
root,worker,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (root,worker,out)) and not out.exists() and not out.is_symlink()
def records(path):
 tree=ast.parse(path.read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
  and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,records(here/'build_qwen38_mixed_recurrent_block_root_image.py'))
pin(worker,records(here/'tests/run_pioneer_q38_mixed_worker_tests.py'))
units=[(root,'boot','RootBoot'),(root,'text','RootText'),(root,'conv','RootConv')]
units += [(worker,name,'Peer'+label) for name,label in (
 ('boot','Boot'),('mixed','Mixed'),('dense','Dense'),('ffn','Ffn'),('service','Output'),('affine','Affine'),('text','Service'),('gdn','Gdn'))]
lines=['/* SPDX-License-Identifier: MIT */','// Mechanically generated from COMPLETE pinned artifacts. Immutable DATA ONLY.',
 '// No native/clock/host/map/journal/source/initialized-state/execution grant.',
 '#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_PROGRAMS_H','#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_PROGRAMS_H',
 '#include "PioneerQ38MixedRecurrentBlockFootprint.h"']
for directory,name,label in units:
 data=(directory/(name+'.bin')).read_bytes();assert data and len(data)%4==0
 lines+=['// SHA256 '+hashlib.sha256(data).hexdigest(),'static const uint32_t PioneerQ38RecurrentBlock'+label+'[]={']
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 lines+=[' '+','.join('0x'+format(w,'08x')+'U' for w in words[i:i+8])+',' for i in range(0,len(words),8)]
 lines+=['};','static_assert(sizeof(PioneerQ38RecurrentBlock'+label+')=='+str(len(data))+',"exact complete '+label+' DATA");']
lines+=['static inline bool PioneerQ38RecurrentBlockProgramWord(unsigned w,uint32_t a,uint32_t& output){',
 ' if(w>=120 || (a&3))return false;',' for(unsigned n=0;n<PioneerQ38RecurrentBlockCodeSpans(w);n++){',
 '  const auto s=PioneerQ38RecurrentBlockCodeSpan(w,n);if(a<s.address || a-s.address>=s.words*4)continue;',
 '  const uint32_t* data=nullptr;','  if(!w)switch(n){']
for i,(_,_,label) in enumerate(units[:3]):lines+=['   case '+str(i)+':data=PioneerQ38RecurrentBlock'+label+';break;']
lines+=['  }else switch(n){']
for i,(_,_,label) in enumerate(units[3:]):lines+=['   case '+str(i)+':data=PioneerQ38RecurrentBlock'+label+';break;']
lines+=['  }','  if(!data){return false;}output=data[(a-s.address)/4];return true;',' }',
 ' return false; // caller output untouched;NO executable padding or runtime grant','}','#endif','']
# A generated artifact,not authored source. Refuse replacement/symlink and
# write only AFTER all roots/peers/ELFs and units have authenticated.
with out.open('x') as target:target.write('\n'.join(lines))
print(hashlib.sha256(out.read_bytes()).hexdigest()+' recurrent-block-programs;DATA ONLY/ALL11 pinned sections/NO native or launch grant')
