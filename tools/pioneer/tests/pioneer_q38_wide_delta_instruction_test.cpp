/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST 1
#include "../PioneerQ38WideDeltaMath.h"
#include "pioneer_q38_wide_delta_instruction_model.h"
#include <cfenv>
int main()
{
 assert(std::fegetround()==FE_TONEAREST);
 unsigned checked=0;
 for(unsigned trial=0;trial<16;trial++){
  WideDeltaUnit m;m.localImage=true;m.Op(0x8a000002);
  PioneerSfpuLoad32(m,6,0);PioneerSfpuLoad32(m,7,0);
  double expected[32]={};
  for(unsigned key=0;key<128;key++){
   for(unsigned lane=0;lane<32;lane++){
    const float a=trial?float(sin((key*32+lane)*.031+trial*.17)*.83):0;
    const float b=trial?float(cos((key+lane)*.123+trial*.317)*.29):0;
    m.lr[0][lane]=Bits(a);m.lr[1][lane]=Bits(b);
    expected[lane]+=double(Float(m.lr[0][lane]))*double(Float(m.lr[1][lane]));
   }
   PioneerQ38WideDeltaProduct(m);PioneerQ38WideDeltaAccumulate(m);
  }
  PioneerQ38VectorAffineAdd(m,6,7,6);m.Drain();
  for(unsigned lane=0;lane<32;lane++){assert(m.lr[6][lane]==Bits(float(expected[lane])));checked++;}
  assert(m.drained && !m.latency && m.splitAnds==256 && m.wideMads==512);
 }
 {
  WideDeltaUnit m;m.localImage=true;m.Op(0x8a000002);m.dropLatencyWait=true;
  bool caught=false;try{PioneerQ38WideDeltaProduct(m);}catch(const WideDeltaUnit::OrderingFault&){caught=true;}
  assert(caught && !m.stores && !m.drains);
 }
 printf("PASS offline actual wide instruction words:16x128x32 products/%u FP64-order rounded sums exact/ONLY8LRegs/explicit latency/missing gap refused;NOT emitted RV32/native/physical/Work/TPS\n",checked);
}
