/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_CHAIN_LAYOUT_H
#define PIONEER_Q38_GLOBAL_CHAIN_LAYOUT_H
#include "PioneerQ38LayerChainLayout.h"
#include "PioneerQ38GlobalWorkerLayout.h"
// DISTINCT QGC1 DATA and proposed device contract; no native launch/token grant.
// Old arithmetic outer/recipe DATA is reused,not old boot/image/owner identity.
namespace PioneerQ38GlobalChainLayout {
namespace Old=PioneerQ38LayerChainLayout;namespace R=Old::R;namespace D=Old::D;namespace P=Old::P;
using Old::Dense;using Old::Bytes;using Old::Address;
using Old::Offset;using Old::Stride;using Old::WorkspaceBytes;
using Old::DenseRecipes;using Old::DenseFfn;using Old::DenseFrames;
enum {Magic=0x31434751,Hidden=5120,Workers=119,PrivateBytes=1028,Layers=64,
 StackBegin=0x6e000,StackEnd=0x70000,Progress=0x75700,
 HeadOffset=0x3000000,HeadBytes=512,Head=0x7f000,Summary=0x75600,
 HeadMagic=0x31485651,CandidateMagic=0x31585651}; // QGC1/QVH1/QVX1 candidate,NOT committed token
using Old::A::PrivateFrames;
static inline unsigned BootWord(unsigned i){return i==40?unsigned(Magic):i==41?1U:i==42?unsigned(Hidden):i==43?unsigned(Workers):i==44?12U:i==45?1U:0U;}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[1]!=1 || b[2]!=64 || b[3]!=129 || b[4]!=b[16] || !b[4] || b[5]!=119 || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<64;i++)if(b[i]!=(i>=40 && i<46?BootWord(i):0U))return false;
 return true;
}
static inline bool Frame(const uint32_t* boot,const uint32_t* outer,bool dense){
 if(!BootShape(boot) || (dense?!D::RunShape(outer):!R::OuterShape(outer)))return false;
 for(unsigned i=0;i<16;i++)if(outer[16+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(outer[32+i]!=boot[8+i])return false;
 return true; // geometry ONLY,not a completed predecessor or native owner
}
static inline bool LayerFrame(const uint32_t* d,unsigned layer,unsigned bank,uint32_t workspace,const uint32_t* boot){
 if(layer>=64 || bank>=7 || !Frame(boot,d,Dense(layer)) || d[3]!=layer || d[4] || d[6]!=1001+layer*16
  || d[7]!=bank || d[9]!=workspace || d[48]!=layer+1)return false;
 uint32_t address=0;if(!Address(workspace,layer,address))return false;
 return Dense(layer)?!d[52] && d[73]==bank && d[74]==workspace+R::FinalOffset:
  !d[12] && !d[13] && (!layer || (d[68]==bank && d[69]==workspace+R::FinalOffset));
}
static_assert(HeadOffset>=Offset+64*Stride && HeadOffset+HeadBytes<=WorkspaceBytes && Head+HeadBytes<=R::Projected,
 "new global immutable metadata and L1 scratch cannot alias layer packages/quantized chunk/output");
}
#endif
