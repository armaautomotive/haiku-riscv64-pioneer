/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Qgw8DenseValueTrace.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 namespace D=PioneerQ38Qgw8DenseValueTrace;namespace G=PioneerQ38Qgw8LayerTraceGeometry;
 for(unsigned base: {0x04000000U,0xc0000000U}){
  uint32_t address=0;G::Chunk first={},next={};
  assert(D::Describe(base,3,0,address) && G::Describe(base,0,first) && G::Describe(base,2,next));
  assert(address==first.address && address+D::Bytes<=next.address);
  assert(!D::Describe(base,2,0,address) && !D::Describe(base,3,1,address));
 }
 uint32_t address=0;
 assert(!D::Describe(0x04000001,3,0,address) && !D::Describe(0x03000000,3,0,address));
 assert(!D::Describe(0xc0001000,3,0,address));
 std::puts("PASS value capture geometry/bounds; NO DEVICE");
}
