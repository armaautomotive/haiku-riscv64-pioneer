/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PUSHED_WORKER_LAYOUT_H
#define PIONEER_Q38_PUSHED_WORKER_LAYOUT_H
#include "PioneerQ38PreciseWorkerLayout.h"
// Distinct proposed DATA only. A matching receipt is NOT a native/physical
// completion or permission to release a job. No ROOT consumer is enabled.
namespace PioneerQ38PushedWorkerLayout {
namespace Base=PioneerQ38PreciseWorkerLayout;
using namespace Base;
enum {Magic=0x33574751,JobWitness=0x334f4751,ReceiptMagic=0x33504d51,
 Root=129,Mailbox=0x118000,Stride=256,ReceiptWords=64,PushBuffer=0x83c00};
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(JobWitness):i==49?unsigned(ReceiptMagic):i==50?unsigned(Root)
  :i==51?unsigned(Mailbox):i==52?unsigned(Stride):i==53?unsigned(PushBuffer)
  :i==54?unsigned(ReceiptWords):Base::BootWord(i);
}
static inline void OldBoot(const uint32_t* b,uint32_t* old){
 for(unsigned i=0;i<64;i++)old[i]=i==0?unsigned(Base::Magic):i==40?unsigned(Base::JobWitness):i>=49 && i<=54?0U:b[i];
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 for(unsigned i=40;i<=54;i++)if(b[i]!=BootWord(i))return false;
 uint32_t old[64];OldBoot(b,old);return Base::BootShape(old);
}
static inline bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || j[0]!=Magic || !BootShape(b))return false;
 uint32_t oldBoot[64],oldJob[64];OldBoot(b,oldBoot);
 for(unsigned i=0;i<64;i++)oldJob[i]=i?j[i]:unsigned(Base::Magic);
 if(j[2]==GlobalMatrix){if(j[31]!=JobWitness)return false;oldJob[31]=Base::JobWitness;}
 return Base::JobShape(oldJob,oldBoot,previous,densePhase);
}
static inline unsigned Slot(unsigned worker){return worker && worker<=119?Mailbox+(worker-1)*Stride:0;}
static inline bool Receipt(const uint32_t* r,const uint32_t* b,const uint32_t* j,const uint32_t* before){
 if(!r || !j || !before || !j[1] || j[2]!=1 || !JobShape(j,b,j[1]-1,0)
  || r[0]!=ReceiptMagic || r[1]!=ReceiptWords || r[2]!=b[3] || r[3]!=j[1]
  || r[4]!=j[5] || r[5]!=j[6] || r[6]!=b[4] || r[7]!=Root)return false;
 for(unsigned i=0;i<32;i++)if(r[8+i]!=j[i])return false;
 for(unsigned i=0;i<16;i++)if(r[40+i]!=b[16+i])return false;
 for(unsigned i=0;i<8;i++)if(r[56+i]!=before[i])return false;
 return before[2]==before[6]; // raw pre-push counter DATA relation,NOT current physical authority.
}
static_assert(Mailbox+119*Stride<=0x120000 && !(Mailbox&63) && !(PushBuffer&63),
 "phase-exclusive existing ROOT proof slots,not a new L1/native mapping grant");
}
#endif
