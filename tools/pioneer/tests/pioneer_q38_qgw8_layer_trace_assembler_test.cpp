/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_QGW8_TRACE_ASSEMBLER_INSPECTION 1
#include "../PioneerQ38Qgw8LayerTraceAssembler.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using A=PioneerQ38Qgw8LayerTraceAssembler;namespace G=PioneerQ38Qgw8LayerTraceGeometry;
int main(){
 constexpr size_t total=64U*5120U;constexpr uint64_t key=0x123456789abcdef0ULL;
 auto assembler=std::make_unique<A>(key,0x04000000);assert(!assembler->Failed());
 std::vector<uint32_t> out(total,0xdeadbeef);assert(!assembler->Copy(out.data(),out.size()) && out[0]==0xdeadbeef);
 std::array<uint32_t,4096> chunk={};
 for(unsigned i=0;i<G::Chunks;i++){
  G::Chunk shape={};assert(G::Describe(0x04000000,i,shape));
  for(unsigned n=0;n<shape.words;n++)chunk[n]=shape.layer*5120+shape.firstWord+n;
  assert(assembler->Append(key,i,chunk.data(),shape.words) && assembler->Cursor()==i+1);
 }
 assert(!assembler->Copy(out.data(),out.size()-1) && out[0]==0xdeadbeef);
 assert(assembler->Copy(out.data(),out.size()));for(unsigned i=0;i<total;i++)assert(out[i]==i);
 assert(!assembler->Copy(out.data(),out.size()) && !assembler->Append(key,128,chunk.data(),1));
 for(unsigned fault=0;fault<6;fault++){
  auto a=std::make_unique<A>(key,0x04000000);out.assign(total,0xdeadbeef);
  if(fault==0)assert(!a->Append(key+1,0,chunk.data(),4096));
  if(fault==1)assert(!a->Append(key,1,chunk.data(),4096));
  if(fault==2)assert(!a->Append(key,0,chunk.data(),1024));
  if(fault==3)assert(!a->Append(key,0,nullptr,4096));
  if(fault==4){assert(a->Append(key,0,chunk.data(),4096));assert(!a->Append(key,0,chunk.data(),4096));}
  if(fault==5)assert(!a->Append(key,128,chunk.data(),4096));
  assert(a->Failed() && !a->Copy(out.data(),out.size()) && out[0]==0xdeadbeef && !a->Append(key,0,chunk.data(),4096));
 }
 A badKey(0,0x04000000),badBase(key,0x04000001);assert(badKey.Failed() && badBase.Failed());
 puts("PASS synthetic 128-chunk ordered trace assembly, six corruption cases, atomic copy/refusal; NO provenance/device/Work");
}
