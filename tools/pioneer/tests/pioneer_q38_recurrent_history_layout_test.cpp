/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38RecurrentHistoryLayout.h"
#include <assert.h>
#include <array>
#include <cstdio>
int main(){
 namespace H=PioneerQ38RecurrentHistoryLayout;unsigned cases=0;
 for(uint32_t base:{0x04000000U,0xc0000000U}){
  std::array<uint32_t,48> address={};unsigned count=0;
  for(unsigned layer=0;layer<64;layer++){
   uint32_t out=0xdeadbeef;bool valid=H::Address(base,layer,out);
   if(layer%4==3){assert(!valid && out==0xdeadbeef);continue;}
   assert(valid && count<48 && out==base+H::Offset+count*H::Stride
    && out>=base && uint64_t(out)+H::Bytes<=uint64_t(base)+67108864 && !(out&4095));
   for(unsigned i=0;i<count;i++)assert(uint64_t(address[i])+H::Bytes<=out);
   address[count++]=out;cases++;
  }assert(count==48);
 }
 for(uint32_t base:{0U,0x03fff000U,0x04000001U,0xc0000001U,0xfffff000U,UINT32_MAX}){
  uint32_t out=0xdeadbeef;assert(!H::Address(base,0,out) && out==0xdeadbeef);cases++;
 }
 for(unsigned layer:{64U,65U,UINT32_MAX}){uint32_t out=0xdeadbeef;assert(!H::Address(0x04000000,layer,out) && out==0xdeadbeef);cases++;}
 printf("PASS recurrent history staging geometry%u/ALL48 disjoint slots/bounded workspace+overflow+atomic refusal;DATA ONLY,NO IO/native/current history or token commit\n",cases);
}
