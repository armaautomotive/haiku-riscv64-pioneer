/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4KernelTiming.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 using T=PioneerQ38Q4KernelTiming;
 unsigned cases=0;
 for(unsigned k:{5120U,17408U})for(unsigned pass=0;pass<2;pass++){
  uint32_t guarded[10]={0x12345678},start[]={0xfffffff0,1},end[]={10,2};guarded[9]=0x87654321;
  auto* r=guarded+1;T::Record(k,pass,start,true,end,true,r);
  assert(T::State(r,k,pass)==1 && guarded[0]==0x12345678 && guarded[9]==0x87654321);cases++;
  for(unsigned field=0;field<8;field++){
   const unsigned saved=r[field];r[field]^=field<4?1U:0xffffffffU;
   if(field<4)assert(!T::State(r,k,pass));
   else{
    const uint64_t a=uint64_t(r[4])|(uint64_t(r[5])<<32),b=uint64_t(r[6])|(uint64_t(r[7])<<32);
    assert(T::State(r,k,pass)==(a && b>a?1U:0U));
   }
   r[field]=saved;cases++;
  }
  for(unsigned which=0;which<5;which++){
   uint32_t a[]={1,1},b[]={2,1};bool av=true,bv=true;
   if(which==0)av=false;if(which==1)bv=false;if(which==2){a[0]=a[1]=0;}
   if(which==3){b[0]=a[0];b[1]=a[1];}if(which==4){b[0]=b[1]=0;}
   T::Record(k,pass,a,av,b,bv,r);assert(T::State(r,k,pass)==2 && !(r[4]|r[5]|r[6]|r[7]));cases++;
  }
 }
 assert(!T::State(nullptr,5120,0));cases++;
 printf("PASS_Q4_KERNEL_TIMING_DATA cases%u wrap1 unavailableNeverInventsCycles1 bounds1;NO_HARDWARE_AUTHORITY\n",cases);
}
