/* SPDX-License-Identifier: MIT */
// Address geometry only. No driver, device, ownership or model evidence.
#include "../PioneerQ38Qgw8LayerTraceGeometry.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
namespace T=PioneerQ38Qgw8LayerTraceGeometry;
int main(){
 unsigned valid=0,refused=0;
 for(uint32_t base:{0x04000000U,0x08000000U,0xc0000000U})for(uint32_t i=0;i<T::Chunks;i++){
  T::Chunk c={};assert(T::Describe(base,i,c));
  assert(c.layer==i/2 && c.part==i%2 && c.firstWord==(i%2)*4096
   && c.words==(i%2?1024U:4096U) && c.address==base+T::Offset+(i/2)*T::Bytes+(i%2)*16384);
  if(i){T::Chunk previous={};assert(T::Describe(base,i-1,previous));assert(previous.address+previous.words*4==c.address);}
  valid++;
 }
 for(uint32_t base:{0U,0x03fff000U,0x04000001U,0xc0000001U,0xfffff000U}){
  T::Chunk c={1,2,3,4,5},before=c;assert(!T::Describe(base,0,c) && !std::memcmp(&c,&before,sizeof(c)));refused++;
 }
 for(uint32_t ordinal:{128U,129U,UINT32_MAX}){
  T::Chunk c={1,2,3,4,5},before=c;assert(!T::Describe(0x04000000,ordinal,c) && !std::memcmp(&c,&before,sizeof(c)));refused++;
 }
 printf("PASS trace geometry %u valid/%u refused, 64 layers/two chunks and address boundaries; NO DEVICE OR OWNERSHIP\n",valid,refused);
}
