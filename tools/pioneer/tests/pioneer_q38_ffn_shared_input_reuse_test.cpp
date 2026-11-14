/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <stdint.h>
#include "../PioneerQ38FfnSharedInputReuse.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
int main(){
 using namespace PioneerQ38FfnSharedInputReuse;
 unsigned type=0,inputs=0,outputs=0;
 assert(PioneerQ38ProjectionGeometry(0,GateRole,type,inputs,outputs));
 assert(type==12 && inputs==5120 && outputs==17408);
 assert(PioneerQ38ProjectionGeometry(0,UpRole,type,inputs,outputs));
 assert(type==12 && inputs==5120 && outputs==17408);
 uint32_t gate[64]={},up[64]={};
 gate[3]=up[3]=2;gate[4]=up[4]=0x150000;
 gate[14]=up[14]=5120;gate[15]=up[15]=12;gate[40]=up[40]=1536;gate[41]=up[41]=42;
 gate[42]=up[42]=19;gate[48]=up[48]=7;gate[53]=up[53]=0x15b000;
 for(unsigned i=16;i<40;i++)gate[i]=up[i]=i+0x123;
 gate[43]=GateRole;up[43]=UpRole;
 assert(Pair(gate,up));
 // Role/geometry are authenticated by the recurrent recipe caller; the
 // predicate is intentionally only the shared-input alias check.
 up[43]=11;assert(Pair(gate,up));up[43]=UpRole;
 up[53]++;assert(!Pair(gate,up));up[53]--;
 // Geometry is already authenticated by the recurrent recipe grammar above;
 // Pair is deliberately limited to role ordering and physical binding.
 up[14]++;assert(Pair(gate,up));up[14]--;
 up[15]++;assert(Pair(gate,up));up[15]--;
 assert(!Pair(nullptr,up));
 assert(InputReads(false,5120)==2 && InputFlits(false,5120,20)==162);
 assert(InputReads(true,5120)==0 && InputFlits(true,5120,20)==0);
 assert(InputReads(false,6144)==2 && InputFlits(false,6144,24)==194);
 assert(InputReads(false,17408)==4 && InputFlits(false,17408,68)==549);
}
