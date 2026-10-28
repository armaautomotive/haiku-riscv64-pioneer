/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error Offline observation bookkeeping is never a native execution grant
#endif
#ifndef PIONEER_Q38_TEST_INSTRUCTION_WINDOWS_H
#define PIONEER_Q38_TEST_INSTRUCTION_WINDOWS_H
#include <cstdint>
// TEST observation only. Rebase the generic interpreter's 32-bit step
// counter after a separately VERIFIED layer, retaining its low8 scheduler
// bits. No PC/register/NoC/Tensix/clock/product state is part of this class.
template<unsigned LayerBound> class PioneerQ38InstructionWindowSet {
 uint64_t retired=0;unsigned closed=0,residue=0;
public:
 static constexpr unsigned Limit=4000000000U,Layers=LayerBound;
 static_assert(Layers && Layers<=64*8192,"bounded observation only, not a model context grant");
 static constexpr uint64_t TotalLimit=uint64_t(Layers+1)*Limit;
 uint64_t Total(unsigned steps)const{return retired+steps;}
 unsigned Closed()const{return closed;}
 bool RebaseAfterVerifiedLayer(unsigned& steps,unsigned verifiedLayers){
  if(closed>=Layers || verifiedLayers!=closed+1 || steps<residue || steps>=Limit || Total(steps)>=TotalLimit)return false;
  const unsigned low=steps&255U;const uint64_t next=retired+steps-low;
  retired=next;steps=low;residue=low;closed=verifiedLayers;return true;
 }
};
using PioneerQ38InstructionWindows=PioneerQ38InstructionWindowSet<64>;
#endif
