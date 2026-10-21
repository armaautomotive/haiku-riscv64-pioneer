#!/usr/bin/env python3
"""Print ONLY apply_patch additions for the independently tested batch4 images."""
import hashlib,pathlib,struct,sys
assert len(sys.argv)==3 or (len(sys.argv)==5 and sys.argv[3]=='--replace' and sys.argv[4] in ('Root','Peer'))
roots=[pathlib.Path(p) for p in sys.argv[1:3]]
pins=[{'text':'68da37b7e5dd672d07fc48a5c1e6f456a59d063cad7eb8613c668cc9fe7194d3',
 'output':'3280945ab6110de738b4e03525388c92f5e02d8d02e785dce3cc448d62741457'},
 {'text':'8ea00560451af37e708f2f45f9660627834a2202b01f5f3ba03969c40e878beb',
 'affine':'7a3f7b2dea02e85129c84b837794642a4f6b85ce4b4fbfdd52f3043697d2b230',
 'fetch':'b6316e76b73f0e9db9bb240cbff5f5104d1b8faba959996428ac16ef56b49c33'}]
images=[]
for root,expected in zip(roots,pins):
 current={}
 for section,pin in {**expected,'boot':'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}.items():
  data=(root/(section+'.bin')).read_bytes()
  assert data and len(data)%4==0 and len(data)<=8192 and hashlib.sha256(data).hexdigest()==pin
  current[section]=data
 assert struct.unpack('<I',current['boot'])[0]==0x0006c06f
 images.append(current)
assert len(images[1]['affine'])<=0x1500 and len(images[1]['fetch'])<=0x700
def array(lines,data,label):
 lines.append(' static const uint32_t '+label+'[]={')
 words=struct.unpack('<'+str(len(data)//4)+'I',data)
 for at in range(0,len(words),8):lines.append('  '+','.join('0x%08xU'%w for w in words[at:at+8])+',')
 lines+=[' };',' static_assert(sizeof('+label+')=='+str(len(data))+',"exact independently pinned batch4 image");']
print('*** Begin Patch')
for index,role in enumerate(('Root','Peer')):
 if len(sys.argv)==5 and role!=sys.argv[4]:continue
 guard='PIONEER_Q38_BATCH4_'+role.upper()+'_IMAGE_H';name='PioneerQ38Batch4'+role
 lines=['/* SPDX-License-Identifier: MIT */','#ifndef '+guard,'#define '+guard,'#include <stdint.h>',
  '// Separate original-layout <=8KiB qualification ONLY;not native/model/speed permission.']
 for section,pin in pins[index].items():lines.append('// '+section+' SHA256 '+pin)
 lines+=['static inline uint32_t '+name+'Word(unsigned word)','{']
 array(lines,images[index]['text'],'code')
 lines+=[' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;','}',
  'static inline uint32_t '+name+'PrefixWord(unsigned address)','{']
 if not index:
  array(lines,images[index]['output'],'code')
  lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
   ' return address-0x70000<sizeof(code)?code[(address-0x70000)/4]:0;']
 else:
  array(lines,images[index]['fetch'],'fetch');array(lines,images[index]['affine'],'affine')
  lines+=[' if(address<0x70000 || address>=0x72000 || (address&3))return UINT32_MAX;',
   ' if(address-0x70000<sizeof(fetch))return fetch[(address-0x70000)/4];',
   ' return address>=0x70b00 && address-0x70b00<sizeof(affine)?affine[(address-0x70b00)/4]:0;']
 lines+=['}','#endif']
 target='src/add-ons/kernel/drivers/misc/'+name+'Image.h'
 if len(sys.argv)==5:
  old=pathlib.Path(target).read_text().splitlines();assert old[0]=='/* SPDX-License-Identifier: MIT */' and old[1]=='#ifndef '+guard
  print('*** Update File: '+target+'\n@@')
  for line in old:print('-'+line)
 else:print('*** Add File: '+target)
 for line in lines:print('+'+line)
print('*** End Patch')
