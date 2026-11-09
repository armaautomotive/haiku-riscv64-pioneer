/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_JOBS_H
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_JOBS_H
#if (!defined(PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION != 1) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 ROOT scheduler is offline inspection only
#endif
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#else
#include <stdint.h>
#endif

// Controller-side geometry for a previously sealed seven-bank table.
// IO.Table(bank,field) reads address/firstRow/rows; Output publishes the
// derived address/bank/firstRow/bands. No source authentication, uploaded
// metadata, worker admission, producer lifetime or launch is implied.
struct PioneerQ38DenseQ8MatrixRootJobs {
 // Matrix bands cover 32 rows; the old scalar recipe instead covers 16.
 // In particular its 64-worker K/V count must never drive this schedule.
 static unsigned Active(unsigned role){return role==5?119U:(role==6 || role==7)?32U:0U;}
 template<class IO> static bool Build(IO& io,unsigned layer,unsigned role){
  if(layer>=64 || layer%4!=3 || role<5 || role>7)return false;
  const unsigned rows=role==5?12288:1024;unsigned address[7],first[7],units[7],quota[7],done[7],emitted[7];
  unsigned minimum=0,total=0;
  for(unsigned b=0;b<7;b++){
   address[b]=io.Table(b,0);first[b]=io.Table(b,1);const unsigned n=io.Table(b,2);
   if(address[b]<0x04000000 || (address[b]&4095) || !n || ((first[b]|n)&31)
    || first[b]>=rows || n>rows-first[b])return false;
   units[b]=n/32;quota[b]=(units[b]+7)/8;done[b]=emitted[b]=0;
   if(units[b]>384 || address[b]>0xc4000000U-units[b]*174080U)return false;
   for(unsigned previous=0;previous<b;previous++)
    if(first[b]<first[previous]+units[previous]*32 && first[previous]<first[b]+n)return false;
   minimum+=quota[b];total+=units[b];
  }
  if(total!=rows/32)return false;
  const unsigned active=total<119?total:119,target=minimum<active?active:minimum;
  if(target!=Active(role))return false;
  for(unsigned extra=minimum;extra<target;extra++){
   unsigned best=7;
   // Each unit count is at most 384 and each quota at most 119, so these
   // products fit in 32 bits and require no RV32 wide-multiply helper.
   for(unsigned b=0;b<7;b++)if(quota[b]<units[b]
    && (best==7 || units[b]*quota[best]>units[best]*quota[b]))best=b;
   if(best==7)return false;quota[best]++;
  }
  uint32_t stage[119][4];unsigned count=0;
  while(count<target){
   bool progress=false;
   for(unsigned b=0;b<7;b++){
    if(emitted[b]==quota[b])continue;
    const unsigned bands=units[b]/quota[b]+unsigned(emitted[b]<units[b]%quota[b]);
    if(!bands || bands>8 || done[b]>units[b] || bands>units[b]-done[b])return false;
    stage[count][0]=address[b]+done[b]*174080U;stage[count][1]=b;
    stage[count][2]=first[b]+done[b]*32;stage[count][3]=bands;count++;
    done[b]+=bands;emitted[b]++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned b=0;b<7;b++){
   if(done[b]!=units[b] || emitted[b]!=quota[b] || io.Table(b,0)!=address[b]
    || io.Table(b,1)!=first[b] || io.Table(b,2)!=units[b]*32)return false;
  }
  for(unsigned w=0;w<count;w++)for(unsigned field=0;field<4;field++)io.Output(w,field,stage[w][field]);
  return true;
 }
};
#endif
