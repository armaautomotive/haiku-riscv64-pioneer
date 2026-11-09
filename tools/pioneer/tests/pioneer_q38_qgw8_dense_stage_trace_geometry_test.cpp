/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Qgw8DenseStageTrace.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 namespace D=PioneerQ38Qgw8DenseStageTrace;
 namespace G=PioneerQ38Qgw8LayerTraceGeometry;
 for(uint32_t workspace:{uint32_t(0x04000000),uint32_t(0x40000000),uint32_t(0xc0000000)}){
  for(uint32_t stage=0;stage<D::Stages;stage++){
   uint32_t address=0;assert(D::Describe(workspace,3,0,stage,address));
   assert(address==workspace+G::Offset+stage*G::Bytes);
   for(uint32_t later=2;later<G::Layers;later++){
    G::Chunk chunk={};assert(G::Describe(workspace,later*2,chunk));
    assert(address+G::Bytes<=chunk.address);
   }
  }
 }
 uint32_t out=0x12345678;
 for(uint32_t layer=0;layer<64;layer++)if(layer!=3){assert(!D::Describe(0x04000000,layer,0,0,out));assert(out==0x12345678);}
 assert(!D::Describe(0x04000000,3,1,0,out) && out==0x12345678);
 assert(!D::Describe(0x04000000,3,0,2,out) && out==0x12345678);
 assert(!D::Describe(0x03fff000,3,0,0,out) && out==0x12345678);
 assert(!D::Describe(0xc0001000,3,0,0,out) && out==0x12345678);
 assert(!D::Describe(0x04000001,3,0,0,out) && out==0x12345678);
 puts("PASS dense layer3 capture geometry and refusals; DATA ONLY, NO CARD OR WORK");
}
