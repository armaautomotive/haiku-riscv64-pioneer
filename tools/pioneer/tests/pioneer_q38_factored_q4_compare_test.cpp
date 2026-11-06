/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_factored_q4_compare_machine.h"
int main(int argc,char** argv){
 assert(argc==2);unsigned words=0;
 for(unsigned width:{5120U,17408U}){
  FactoredCompareData data(width,0);auto vm=std::make_unique<FactoredCompareRV32>(argv[1],width);data.Stage(vm->bus);
  assert(vm->Execute());std::array<unsigned,32> factored{};
  for(const auto& block:data.blocks)assert(PioneerQ38FactoredQ4Reference::Block(block.original.data(),block.input.data(),factored.data()));
  for(unsigned row=0;row<32;row++){
   assert(vm->ReadL1(SL::Output+row*4)==data.expected[row]);
   assert(vm->ReadL1(SL::Output+128+row*4)==factored[row]);words+=2;
  }
  printf("PASS_COMPILED_COMPARE K%u originalSameBytes_oldBF16_newFactored independentReferences64 wholeSharedContext_NoC_PACK_FPU_SFPU;SP%u_touched%u;SYNTHETIC_CLOCK_NOT_HARDWARE_SPEED\n",width,0x70000-vm->minSP,0x70000-vm->minStore);
 }
 assert(words==128);static_cast<void>(&NoMore);
}
