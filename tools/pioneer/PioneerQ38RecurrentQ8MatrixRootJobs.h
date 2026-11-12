/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_JOBS_H
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_JOBS_H
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 ROOT scheduler is offline inspection only
#endif
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif

// Derive matrix-engine jobs from a previously sealed seven-bank stripe table.
// This does not authenticate the table, upload weights, admit workers or launch.
// A caller must establish those properties before this can be used on a device.
struct PioneerQ38RecurrentQ8MatrixRootJobs {
 template<class IO> static bool Build(IO& io,unsigned layer,unsigned role){
  if(layer>=64 || layer%4==3 || (role!=5 && role!=6 && role!=13))return false;
  const unsigned rows=role==5?6144:role==6?10240:5120;
  const unsigned bandBytes=role==13?208896:174080;
  unsigned address[7],first[7],units[7],quota[7],done[7],emitted[7];
  unsigned minimum=0,total=0;
  for(unsigned b=0;b<7;b++){
   address[b]=io.Table(b,0);first[b]=io.Table(b,1);const unsigned n=io.Table(b,2);
   if(address[b]<0x04000000 || (address[b]&4095) || !n || ((first[b]|n)&31)
    || first[b]>=rows || n>rows-first[b])return false;
   units[b]=n/32;quota[b]=(units[b]+7)/8;done[b]=emitted[b]=0;
   if(units[b]>320 || address[b]>0xc4000000U-units[b]*bandBytes)return false;
   for(unsigned p=0;p<b;p++)
    if(first[b]<first[p]+units[p]*32 && first[p]<first[b]+n)return false;
   minimum+=quota[b];total+=units[b];
  }
  if(total!=rows/32 || minimum>119 || total<119)return false;
  for(unsigned extra=minimum;extra<119;extra++){
   unsigned best=7;
   for(unsigned b=0;b<7;b++)if(quota[b]<units[b]
    && (best==7 || units[b]*quota[best]>units[best]*quota[b]))best=b;
   if(best==7)return false;
   quota[best]++;
  }
  // Keep the whole candidate schedule private until validation finishes,
  // but encode bank/band count compactly to fit the controller stack.
  uint32_t stage[119][2];unsigned count=0;
  while(count<119){
   bool progress=false;
   for(unsigned b=0;b<7;b++){
    if(emitted[b]==quota[b])continue;
    // Avoid a variable divide in the eventual RV32 controller image.
    unsigned base=0,remainder=units[b];
    while(remainder>=quota[b]){remainder-=quota[b];base++;}
    const unsigned bands=base+unsigned(emitted[b]<remainder);
    if(!bands || bands>8 || done[b]>units[b] || bands>units[b]-done[b])return false;
    stage[count][0]=first[b]+done[b]*32;stage[count][1]=b|(bands<<3);count++;
    done[b]+=bands;emitted[b]++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned b=0;b<7;b++)
   if(done[b]!=units[b] || emitted[b]!=quota[b] || io.Table(b,0)!=address[b]
    || io.Table(b,1)!=first[b] || io.Table(b,2)!=units[b]*32)return false;
  for(unsigned w=0;w<119;w++){
   const unsigned row=stage[w][0],bank=stage[w][1]&7,bands=stage[w][1]>>3;
   io.Output(w,0,address[bank]+((row-first[bank])/32)*bandBytes);
   io.Output(w,1,bank);io.Output(w,2,row);io.Output(w,3,bands);
  }
  return true;
 }
};
#endif
