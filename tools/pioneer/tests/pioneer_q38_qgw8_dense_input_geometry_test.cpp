/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Qgw8DenseInputTrace.h"
#include <cassert>
int main(){
 namespace D=PioneerQ38Qgw8DenseInputTrace;
 uint32_t addr=0;
 assert(D::Describe(0x04000000,3,0,addr) && addr==0x07500000);
 assert(D::Describe(0xc0000000,3,0,addr) && addr==0xc3500000);
 const uint32_t saved=addr;
 assert(!D::Describe(0x03fff000,3,0,addr) && addr==saved);
 assert(!D::Describe(0xc0001000,3,0,addr) && addr==saved);
 assert(!D::Describe(0x04000001,3,0,addr) && addr==saved);
 assert(!D::Describe(0x04000000,2,0,addr) && addr==saved);
 assert(!D::Describe(0x04000000,3,1,addr) && addr==saved);
 assert(D::Bytes==24576 && D::Bytes<2*PioneerQ38Qgw8LayerTraceGeometry::Bytes);
}
