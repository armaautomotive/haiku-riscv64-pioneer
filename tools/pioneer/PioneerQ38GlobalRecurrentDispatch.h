/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_RECURRENT_DISPATCH_H
#define PIONEER_Q38_GLOBAL_RECURRENT_DISPATCH_H
#include "PioneerQ38GlobalWorkerLayout.h"
#include "PioneerQ38MixedRecurrentDispatch.h"
// Reuse numeric capsule DATA,not the old QMW1 envelope/boot/source authority.
namespace PioneerQ38GlobalRecurrentDispatch {
namespace M=PioneerQ38GlobalWorkerLayout;namespace Old=PioneerQ38MixedRecurrentDispatch;
using Old::Internal;using Old::DisjointOutput;
static inline bool Binding(const uint32_t* b,const uint32_t* d){
 if(!b || !d || ((reinterpret_cast<__UINTPTR_TYPE__>(b)|reinterpret_cast<__UINTPTR_TYPE__>(d))&3) || !M::BootShape(b))return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=b[16+i])return false;
 for(unsigned i=0;i<8;i++)if(d[32+i]!=b[8+i])return false;
 return true;
}
static inline bool Envelope(const uint32_t* b,const uint32_t* d,const uint32_t* internal,unsigned previous,uint32_t* output){
 if(!output)return false;
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=0;
 const unsigned head[]={M::Magic,internal[12],internal[13],b[3],b[4],d[5],d[40],d[6]};
 for(unsigned i=0;i<8;i++){stage[i]=head[i];stage[8+i]=b[8+i];}
 for(unsigned i=0;i<10;i++)stage[16+i]=internal[i];
 if(!M::JobShape(stage,b,previous,0))return false;
 for(unsigned i=0;i<64;i++)output[i]=stage[i];
 return true;
}
static inline bool Conv(const uint32_t* b,const uint32_t* table,unsigned previous,uint32_t* output){
 if(!Binding(b,table) || previous>=0x7ffffffe || !DisjointOutput(output,b,64) || !DisjointOutput(output,table,384))return false;
 uint32_t internal[16];Internal(7,table[6],b[3],previous+1,internal);
 return PioneerQ38ResidentConvServiceJob(internal,table,b+16,b[3],b[4]) && Envelope(b,table,internal,previous,output);
}
static inline bool Gdn(const uint32_t* b,const uint32_t* input,const uint32_t* state,const uint32_t* gated,unsigned previous,uint32_t* output){
 if(!Binding(b,gated) || previous>=0x7ffffffe || !DisjointOutput(output,b,64) || !DisjointOutput(output,input,64)
  || !DisjointOutput(output,state,64) || !DisjointOutput(output,gated,64))return false;
 uint32_t internal[16];Internal(8,gated[6],b[3],previous+1,internal);
 return PioneerQ38ResidentGdnServiceJob(internal,gated,input,state,b+16,b[3],b[4]) && Envelope(b,gated,internal,previous,output);
}
}
#endif
