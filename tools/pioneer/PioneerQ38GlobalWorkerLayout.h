/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_WORKER_LAYOUT_H
#define PIONEER_Q38_GLOBAL_WORKER_LAYOUT_H
#include "PioneerQ38MixedWorkerLayout.h"
#include "PioneerQwen38Panel.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// DISTINCT proposed worker DATA ABI. Not a new mode of an old live cohort.
// New immutable image, fresh ALL119 boot and own native owner are required.
namespace PioneerQ38GlobalWorkerLayout {
namespace Old=PioneerQ38MixedWorkerLayout;
enum {Magic=0x31574751,Fault=0x46574751,GlobalMatrix=12,GlobalLayer=64,
 Hidden=5120,Vocabulary=248320,Waves=9,Partial=0x83e00,PartialWords=64,
 PartialMagic=0x31504751,JobWitness=0x314f4751}; // QGW1/QGP1/QGO1
using Old::Peer;using Old::Boot;using Old::Job;using Old::Doorbell;using Old::Status;
using Old::Code;using Old::DenseCode;using Old::DenseScores;using Old::DenseExp;using Old::DenseValues;using Old::Stop;
struct PrivateSeal {uint32_t boot[64],job[64];};
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(JobWitness):i==41?1U:i==42?unsigned(Hidden):i==43?unsigned(Vocabulary):i==44?unsigned(GlobalMatrix):i==45?1U:0U;
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];
 for(unsigned i=0;i<64;i++){
  if(i>=40 && i<46 && b[i]!=BootWord(i))return false;
  old[i]=i>=40 && i<46?0U:b[i];
 }
 old[0]=Old::Magic;return Old::BootShape(old);
}
static inline bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 const bool globalStop=j[2]==Stop && j[5]==GlobalLayer;
 if(j[2]!=GlobalMatrix && !globalStop){
  uint32_t oldBoot[64],oldJob[64];
  for(unsigned i=0;i<64;i++){oldBoot[i]=i>=40 && i<46?0U:b[i];oldJob[i]=j[i];}
  oldBoot[0]=oldJob[0]=Old::Magic;return Old::JobShape(oldJob,oldBoot,previous,densePhase);
 }
 if(densePhase || previous>=0x7ffffffe || j[1]!=previous+1 || !j[1] || j[1]>=0x7fffffff
  || j[3]!=b[3] || j[4]!=b[4] || j[5]!=GlobalLayer || j[6]>=8192 || !j[7] || j[7]>=0x80000000)return false;
 for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
 if(globalStop){for(unsigned i=16;i<64;i++)if(j[i])return false;return true;}
 if(j[16]!=0x31483851 || j[17]!=14 || j[18]!=3392 || j[19] || j[20] || j[21]!=Hidden
  || j[22]<0x04000000 || (j[22]&63) || !j[23] || j[23]>16 || j[25]!=1 || j[26]!=1
  || j[27]>=Waves || (j[27]==8 && b[3]>21) || (j[28]&15) || j[28]>=Vocabulary || j[23]*16>Vocabulary-j[28]
  || j[29]!=Vocabulary || j[30]!=1 || j[31]!=JobWitness)return false;
 unsigned bytes=j[23]*20*3392;if(j[22]>0xc4000000-bytes)return false;
 bool route=false;for(unsigned bank=0;bank<7;bank++)route |= j[24]==PioneerQ38ProjectionRoute(bank);
 if(!route)return false;
 for(unsigned i=32;i<64;i++)if(j[i])return false;
 return true; // geometry ONLY, not canonical rows/source/weight/init authority
}
static inline unsigned LogitKey(unsigned bits){
 if(!(bits&0x7fffffff))bits=0;
 return bits&0x80000000?~bits:bits^0x80000000;
}
// Arithmetic ONLY. No launch, completed layer, wave-coverage or token grant.
// First maximum globally, independent of bank/wave order, including +/-0.
static inline bool Better(unsigned bits,unsigned row,unsigned bestBits,unsigned bestRow){
 return LogitKey(bits)>LogitKey(bestBits) || (LogitKey(bits)==LogitKey(bestBits) && row<bestRow);
}
static_assert(Partial+PartialWords*4==0x83f00 && sizeof(PrivateSeal)==512,"separate partial precedes existing matrix proof;no native mapping grant");
}
#endif
