/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LAYER_CHAIN_LAYOUT_H
#define PIONEER_Q38_LAYER_CHAIN_LAYOUT_H
#include "PioneerQ38LayerReturnComponentLayout.h"
#include "PioneerQ38RecurrentPrefixLayout.h"
// All64 immutable package DATA; no source/producer/native/token authority.
namespace PioneerQ38LayerChainLayout {
namespace A=PioneerQ38LayerReturnComponentLayout;
namespace R=A::R;namespace D=A::D;namespace P=PioneerQ38RecurrentPrefixLayout;
enum {Magic=0x31484351,Layers=64,Offset=0x1000000,Stride=0x80000,WorkspaceBytes=67108864,
 DenseRecipes=512,DenseFfn=DenseRecipes+4*D::RecipeStride,
 DenseFrames=DenseRecipes+7*D::RecipeStride,DenseBytes=DenseFrames+96*256,
 Progress=0x75700}; // QCH1; distinct from any native/old prefix result
static inline bool Dense(unsigned layer){return layer<Layers && layer%4==3;}
static inline unsigned Bytes(unsigned layer){return layer<Layers?(Dense(layer)?unsigned(DenseBytes):unsigned(P::Bytes)):0;}
static inline bool Address(uint32_t workspace,unsigned layer,uint32_t& output){
 if((workspace&4095) || workspace<0x04000000 || workspace>0xc0000000 || layer>=Layers)return false;
 output=workspace+Offset+layer*Stride;return true;
}
static inline bool Frame(const uint32_t* d,unsigned layer,unsigned bank,uint32_t workspace,const uint32_t* boot){
 if(layer>=Layers || bank>=7 || !A::Frame(boot,d,Dense(layer)) || d[3]!=layer || d[4]
  || d[6]!=1001+layer*16 || d[7]!=bank || d[9]!=workspace || d[48]!=layer+1)return false;
 uint32_t address=0;if(!Address(workspace,layer,address))return false;
 if(Dense(layer))return !d[52] && d[73]==bank && d[74]==workspace+R::FinalOffset;
 return !d[12] && !d[13] && (!layer || (d[68]==bank && d[69]==workspace+R::FinalOffset));
} // original input/state and canonical metadata STILL need independent owners
static_assert(Offset+Layers*Stride<=WorkspaceBytes && unsigned(P::Bytes)<=unsigned(Stride) && DenseBytes<=Stride,
 "all64 bounded package slots fit the existing reserved workspace");
static_assert(PioneerQ38RecurrentHistoryLayout::Offset+PioneerQ38RecurrentHistoryLayout::Layers*PioneerQ38RecurrentHistoryLayout::Stride<=Offset,
 "all provisional histories precede immutable metadata slots");
static_assert(!(DenseRecipes&63) && !(DenseFfn&63) && !(DenseFrames&63) && !(DenseBytes&63),"aligned exact packet regions");
static_assert(unsigned(D::StopSummary)+256<=Progress && Progress+256<=unsigned(D::Parts),"distinct chain progress,not an old STOP/result alias");
}
#endif
