/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LAYER_RETURN_COMPONENT_LAYOUT_H
#define PIONEER_Q38_LAYER_RETURN_COMPONENT_LAYOUT_H
#include "PioneerQ38MixedRecurrentBlockRootLayout.h"
#include "PioneerQ38MixedLiveBlockRootLayout.h"
// DATA ABI for a future common on-card scheduler. Not a scheduler, initialized
// input/state producer, completed layer, native lifetime or token-commit seal.
namespace PioneerQ38LayerReturnComponentLayout {
namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
namespace D=PioneerQ38MixedLiveBlockRootLayout;
enum {Magic=0x31434c51,Hidden=5120,Workers=119,PrivateBytes=1028,StackBegin=0x6e000,StackEnd=0x70000}; // QLC1, not QRB1/QBL1/QPX1
static inline bool PrivateFrames(uint32_t parent,uint32_t local){
 if(((parent|local)&3) || parent<StackBegin || local<StackBegin
  || parent>StackEnd-PrivateBytes || local>StackEnd-PrivateBytes)return false;
 return parent+PrivateBytes<=local || local+PrivateBytes<=parent;
} // minimum private seal ranges ONLY; not caller/native lifetime authority
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(Magic):i==41?1U:i==42?unsigned(Hidden):i==43?unsigned(Workers):i==44?2U:i==45?1U:0U;
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[4]!=b[16])return false;
 uint32_t old[64];
 for(unsigned i=0;i<64;i++){
  if(i>=40 && i<46 && b[i]!=BootWord(i))return false;
  old[i]=i>=40 && i<46?0U:b[i];
 }
 old[0]=R::Magic;
 return R::BootShape(old);
}
static inline bool Frame(const uint32_t* boot,const uint32_t* outer,bool dense){
 if(!BootShape(boot) || (dense?!D::RunShape(outer):!R::OuterShape(outer)))return false;
 for(unsigned i=0;i<16;i++)if(outer[16+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(outer[32+i]!=boot[8+i])return false;
 return true; // geometry ONLY; caller must separately own the real predecessor
}
static_assert(unsigned(R::Boot)==unsigned(D::Boot) && unsigned(R::Outer)==unsigned(D::Outer) && unsigned(R::Header)==unsigned(D::Header)
 && unsigned(R::Doorbell)==unsigned(D::Doorbell) && unsigned(R::PeerBoots)==unsigned(D::PeerBoots) && unsigned(R::PeerJobs)==unsigned(D::PeerJobs)
 && unsigned(R::Nonces)==unsigned(D::Nonces) && unsigned(R::Counters)==unsigned(D::Counters) && unsigned(R::FinalOffset)==unsigned(D::FinalOffset),
 "returning layers must retain the same worker chains and GDDR hidden slot");
static_assert(sizeof(R::PrivateSeal)==PrivateBytes && sizeof(D::PrivateSeal)==PrivateBytes,"separate, equally bounded private snapshot types");
}
#endif
