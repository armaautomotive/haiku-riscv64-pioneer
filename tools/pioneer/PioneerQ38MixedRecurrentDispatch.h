/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_DISPATCH_H
#define PIONEER_Q38_MIXED_RECURRENT_DISPATCH_H
#include "PioneerQ38MixedWorkerLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentConvServiceShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnServiceShape.h"
// DATA-only bridge for the future mixed recurrent coordinator. The outer
// mailbox is QMW1, NOT the old QFP2 external job/doorbell/status ABI. The
// same old numeric capsules are constructed privately by the QMW1 worker.
// observedPrevious MUST come from the owner's coherent live closed-chain
// admission. This helper cannot prove it, set a worker nonce, release a
// worker, initialize state, select code, or confer native/Work authority.
namespace PioneerQ38MixedRecurrentDispatch {
namespace M=PioneerQ38MixedWorkerLayout;
static inline bool DisjointOutput(const uint32_t* output,const uint32_t* input,unsigned words){
 using Address=__UINTPTR_TYPE__;Address o=reinterpret_cast<Address>(output),i=reinterpret_cast<Address>(input),max=~Address(0);
 return output && input && !((o|i)&3) && o<=max-256 && i<=max-Address(words)*4
  && (o+256<=i || i+Address(words)*4<=o);
} // pointer arithmetic only,NOT an OS mapping/readability or ownership check
static inline bool Binding(const uint32_t* b,const uint32_t* d){
 if(!b || !d || ((reinterpret_cast<__UINTPTR_TYPE__>(b)|reinterpret_cast<__UINTPTR_TYPE__>(d))&3) || !M::BootShape(b))return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=b[16+i])return false;
 for(unsigned i=0;i<8;i++)if(d[32+i]!=b[8+i])return false;
 return true;
}
static inline void Internal(unsigned operation,unsigned phase,unsigned worker,unsigned next,uint32_t* j){
 for(unsigned i=0;i<16;i++)j[i]=0;
 j[0]=operation==7?uint32_t(TT_Q38_RESIDENT_CONV_SERVICE_JOB):uint32_t(TT_Q38_RESIDENT_GDN_SERVICE_JOB);
 j[1]=operation==7?PioneerQ38ConvBatchCount(worker):64;j[2]=operation==7?1536:768;j[3]=phase;
 j[10]=TT_Q38_FFN_PERSISTENT_MARKER;j[11]=worker;j[12]=next;j[13]=operation;
}
static inline bool Envelope(const uint32_t* b,const uint32_t* d,const uint32_t* internal,
 unsigned previous,uint32_t* output){
 if(!output)return false;
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=0;
 const unsigned head[]={M::Magic,internal[12],internal[13],b[3],b[4],d[5],d[40],d[6]};
 for(unsigned i=0;i<8;i++){stage[i]=head[i];stage[8+i]=b[8+i];}
 for(unsigned i=0;i<10;i++)stage[16+i]=internal[i];
 if(!M::JobShape(stage,b,previous,0))return false;
 for(unsigned i=0;i<64;i++)output[i]=stage[i];
 return true;
}
static inline bool Conv(const uint32_t* boot,const uint32_t* table,unsigned observedPrevious,uint32_t* output){
 if(!Binding(boot,table) || observedPrevious>=0x7ffffffe || !DisjointOutput(output,boot,64)
  || !DisjointOutput(output,table,384))return false;
 uint32_t internal[16];Internal(7,table[6],boot[3],observedPrevious+1,internal);
 if(!PioneerQ38ResidentConvServiceJob(internal,table,boot+16,boot[3],boot[4]))return false;
 return Envelope(boot,table,internal,observedPrevious,output);
}
static inline bool Gdn(const uint32_t* boot,const uint32_t* input,const uint32_t* state,const uint32_t* gated,
 unsigned observedPrevious,uint32_t* output){
 if(!Binding(boot,gated) || observedPrevious>=0x7ffffffe || !DisjointOutput(output,boot,64)
  || !DisjointOutput(output,input,64) || !DisjointOutput(output,state,64) || !DisjointOutput(output,gated,64))return false;
 uint32_t internal[16];Internal(8,gated[6],boot[3],observedPrevious+1,internal);
 if(!PioneerQ38ResidentGdnServiceJob(internal,gated,input,state,boot+16,boot[3],boot[4]))return false;
 return Envelope(boot,gated,internal,observedPrevious,output);
}
}
#endif
