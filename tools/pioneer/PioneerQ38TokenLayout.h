/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_LAYOUT_H
#define PIONEER_Q38_TOKEN_LAYOUT_H
#include "PioneerQ38GlobalHeadLayout.h"
#include "PioneerQ38TokenStateLayout.h"
// Experimental persistent-parent DATA. A native owner still owes source
// authentication, initialized/reserved pages, exclusive lifetimes and IO.
namespace PioneerQ38TokenLayout {
namespace C=PioneerQ38GlobalChainLayout;namespace H=PioneerQ38GlobalHeadLayout;
enum {Magic=0x31545351,TokenMagic=0x31435451,TerminalMagic=0x31545451,
 SessionOffset=0x3100000,PromptOffset=0x3180000,OutputOffset=0x3200000,
 SessionLocal=C::Head+1536,RecordLocal=C::Head+2048,TerminalLocal=C::Summary,
 SessionBytes=512,RecordBytes=256,Context=8192};
static inline bool Session(const uint32_t* d,const uint32_t* boot,unsigned bank,unsigned workspace){
 if(!d || !C::BootShape(boot) || bank>=7 || workspace<0x04000000 || workspace>0xc0000000 || (workspace&4095)
  || !d[5] || d[5]>Context || !d[6] || d[6]>d[5] || d[7]>=248320 || d[32]>1
  || d[33]>=248320 || d[34]>=248320)return false;
 const unsigned head[]={Magic,1,128,bank,workspace,d[5],d[6],d[7]};
 for(unsigned i=0;i<8;i++)if(d[i]!=head[i])return false;
 for(unsigned i=0;i<16;i++)if(d[8+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(d[24+i]!=boot[8+i])return false;
 if(d[35]!=Context)return false;
 for(unsigned i=36;i<128;i++)if(d[i])return false;
 return true;
}
static inline bool Job(const uint32_t* h,unsigned position,unsigned ordinal,unsigned previous,const uint32_t* boot,uint32_t* output){
 if(position>=Context || !H::Job(h,ordinal,previous,boot,output))return false;
 output[6]=position;output[7]+=position*4096;
 return H::G::JobShape(output,boot,previous,0);
}
static_assert(SessionOffset>=C::HeadOffset+C::HeadBytes && SessionOffset+SessionBytes<=PromptOffset
 && PromptOffset+Context*4<=OutputOffset && OutputOffset+Context*RecordBytes<=C::WorkspaceBytes,
 "persistent session, immutable prompt and token stream cannot alias existing workspace extents");
static_assert(PioneerQ38TokenStateLayout::Local+256<=SessionLocal && SessionLocal+512<=RecordLocal
 && RecordLocal+256<=C::R::Projected,"separate L1 state/session/record storage");
}
#endif
