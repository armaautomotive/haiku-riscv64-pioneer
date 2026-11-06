/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_prepared_bf16_q4_compare_machine.h"
int main(int argc,char** argv){
 assert(argc==2);static_cast<void>(&NoMore);unsigned words=0,refused=0;
 for(unsigned width:{5120U,17408U}){
  PreparedCompareData data(width,0);auto vm=std::make_unique<PreparedCompareRV32>(argv[1],width);data.Stage(vm->bus);
  const auto originalDram=vm->bus.dram;assert(vm->Execute());std::array<unsigned,32> expected{};
  for(const auto& block:data.blocks)for(unsigned g=0;g<8;g++)assert(PioneerQ38PreparedBf16Q4Reference::Group(block.original.data(),block.input.data(),g,expected.data()));
  for(unsigned row=0;row<32;row++){
   assert(vm->ReadL1(SL::Output+row*4)==data.expected[row]);assert(vm->ReadL1(SL::Output+128+row*4)==expected[row]);words+=2;
  }
  assert(vm->bus.dram==originalDram && vm->bus.overlap && vm->native.maskReads==width/256*256 && vm->native.escapeWrites==width/256*512);
  printf("PASS_COMPILED_PREPARED_BF16_COMPARE K%u oldAndNew64 exactOriginalByteReferences samePhysicalDst_L1_threePacketPreparedSource_26PacketOld_doubleBufferOverlap countersWrap;RV%u_SP%u_touched%u;SYNTHETIC_CLOCK_NOT_SILICON_CYCLES_OR_TPS\n",
   width,vm->steps,0x70000-vm->minSP,0x70000-vm->minStore);
 }
 for(unsigned word=0;word<8;word++){
  auto vm=std::make_unique<PreparedCompareRV32>(argv[1],5120);
  vm->WriteL1(SL::Frame+word*4,word==3?0:vm->ReadL1(SL::Frame+word*4)^1);
  assert(!vm->Execute() && !vm->bus.issues && !vm->native.matrixOps && !vm->stores);refused++;
 }
 for(unsigned fault:{1U,5U,11U,13U}){
  PreparedCompareData data(5120,0);auto vm=std::make_unique<PreparedCompareRV32>(argv[1],5120);data.Stage(vm->bus);
  vm->lateFault=fault;assert(!vm->Execute() && vm->stores==32 && !vm->native.panels);
  for(unsigned row=0;row<32;row++)assert(vm->ReadL1(SL::Output+row*4)==data.expected[row]);refused++;
 }
 assert(words==128 && refused==12);
 printf("PASS_COMPILED_PREPARED_BF16_REFUSALS %u invalidFramesZeroIO_orBadSourceCounterCreditCommand_neverPublishNewOutput;OFFBOARD_NOT_NATIVE_OWNER_FAULT_INJECTION\n",refused);
}
