#!/usr/bin/env python3
"""Generate complete pinned BF16 code DATA only;never install or launch."""
import hashlib
import os
from pathlib import Path
import sys
from generate_qwen38_token_programs import authenticated, require_complete_sections
from PioneerQ38Bf16ProgramPins import ROOT_PINS, WORKER_PINS

assert __debug__ and len(sys.argv)==4
root, worker, out=map(Path,sys.argv[1:])
assert all(p.is_absolute() for p in (root,worker,out)) and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
r=authenticated(root,ROOT_PINS);w=authenticated(worker,WORKER_PINS)
units=[(r,'boot','RootBoot',0x6000),(r,'bf16','RootBf16',0x20000),(r,'text','RootText',0x40000),
       (r,'terminal','RootTerminal',0x60000),(r,'conv','RootAux',0x64000)]
units += [(w,name,label,address) for name,label,address in (
 ('boot','PeerBoot',0x6000),('globalworker','PeerFront',0x20000),('bf16producer','PeerBf16',0x30000),
 ('dense','PeerDense',0x40000),('cachedfpu','PeerFpu',0x50000),('ffn','PeerFfn',0x60000),
 ('service','PeerService',0x68000),('affine','PeerAffine',0x70b00),('text','PeerText',0x72000),('gdn','PeerGdn',0xb8000))]
require_complete_sections(units,(r,w))
lines=['/* SPDX-License-Identifier: MIT */','// Generated complete QTP4/QGW4 immutable code DATA;no runtime qualification.',
 '#include "PioneerQ38TokenBf16ProgramsRole.h"','#ifndef PIONEER_Q38_TOKEN_BF16_PROGRAMS_H',
 '#define PIONEER_Q38_TOKEN_BF16_PROGRAMS_H','#include "PioneerQ38TokenBf16Footprint.h"',
 'namespace PioneerQ38TokenBf16Programs {',
 'enum {Family=0x34505451,Version=1,CancelBoundary=1,Bf16=1,EogCount=5};']
for source,name,label,address in units:
 data=source[name+'.bin'];assert data and not len(data)%4
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 lines += ['// SHA256 '+hashlib.sha256(data).hexdigest(),'static const uint32_t '+label+'[]={']
 lines += [' '+','.join('0x%08xU'%v for v in words[i:i+8])+',' for i in range(0,len(words),8)]
 lines += ['};','static_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");']
lines += ['using PioneerQ38TokenBf16L1::Span;',
 'static inline unsigned CodeSpans(unsigned worker){return worker>=120?0:worker?10:5;}',
 'static inline Span CodeSpan(unsigned worker,unsigned span){',
 ' if(worker>=120 || span>=CodeSpans(worker))return {UINT32_MAX,0};',' if(!worker)switch(span){']
for i,(_,_,label,address) in enumerate(units[:5]):lines.append('  case %u:return {0x%x,unsigned(sizeof(%s)/4)};'%(i,address,label))
lines += [' }else switch(span){']
for i,(_,_,label,address) in enumerate(units[5:]):lines.append('  case %u:return {0x%x,unsigned(sizeof(%s)/4)};'%(i,address,label))
lines += [' }return {UINT32_MAX,0};','}',
 'static inline unsigned CodeWords(unsigned worker){unsigned n=0;for(unsigned i=0;i<CodeSpans(worker);i++)n+=CodeSpan(worker,i).words;return n;}',
 'static inline uint32_t CodeAddress(unsigned worker,unsigned ordinal){',
 ' if(worker>=120)return UINT32_MAX;',
 ' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;}',
 ' return UINT32_MAX;','}',
 'static inline bool ProgramWord(unsigned worker,uint32_t address,uint32_t& output){',
 ' if(worker>=120 || (address&3) || (reinterpret_cast<uintptr_t>(&output)&3))return false;',
 ' const uintptr_t target=reinterpret_cast<uintptr_t>(&output);']
for _,_,label,_ in units:
 lines.append(' if(target>=reinterpret_cast<uintptr_t>(%s) && target-reinterpret_cast<uintptr_t>(%s)<sizeof(%s))return false;'%(label,label,label))
lines += [' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
 '  if(address<s.address || address-s.address>=s.words*4)continue;',
 '  const uint32_t* data=nullptr;',
 '  if(!worker)switch(i){']
for i,(_,_,label,_) in enumerate(units[:5]):lines.append('   case %u:data=%s;break;'%(i,label))
lines += ['  }else switch(i){']
for i,(_,_,label,_) in enumerate(units[5:]):lines.append('   case %u:data=%s;break;'%(i,label))
lines += ['  }if(!data)return false;output=data[(address-s.address)/4];return true;',
 ' }return false; // no implicit executable padding or runtime-data access','}',
 '}','#endif','static_assert(PioneerQ38TokenBf16Programs::Family==0x34505451,"late QTP4 code DATA selection");']
out.write_text('\n'.join(lines)+'\n')
print('PASS COMPLETE BF16 code DATA15 sections/two authenticated whole ELFs/SHA256 '+hashlib.sha256(out.read_bytes()).hexdigest()+';NOT execution/native authority')
