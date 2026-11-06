/* SPDX-License-Identifier: MIT */
#define PIONEER_Q4_KERNEL_TIMING_MACHINE_ONLY
#include "pioneer_q38_q4_kernel_timing_image_test.cpp"
static void Park(Q4TimingRV32& image){
 assert(image.pc==0x70058 && !image.stores && !image.timerStores && !image.native.outputReads);
 const unsigned reads=image.bus.reads,writes=image.bus.writes,math=image.native.matrixOps,issues=image.bus.issues;
 for(unsigned i=0;i<2048;i++){image.TimingStep();image.Progress();}
 assert(image.pc==0x70058 && image.bus.reads==reads && image.bus.writes==writes
  && image.bus.issues==issues && image.native.matrixOps==math);
}
int main(int argc,char** argv){
 assert(argc==2);static_cast<void>(&NoMore);CompactStreamData data(5120,0);unsigned refused=0,numeric=0;
 for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],5120);data.Stage(image->bus);image->bus.fault=fault;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned field=0;field<8;field++){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],5120);data.Stage(image->bus);
  const unsigned value=image->ReadL1(SL::Frame+field*4);
  image->WriteL1(SL::Frame+field*4,field<4?value^1:field==4?0:1);
  assert(!image->Execute() && !image->clockReads && !image->bus.reads && !image->bus.writes);Park(*image);refused++;
 }
 for(unsigned config:{72U,112U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],5120);data.Stage(image->bus);image->dropConfig=config;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned ack:{1U,100U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],5120);data.Stage(image->bus);image->dropAckAt=ack;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned phase=0;phase<4;phase++){
  auto image=std::make_unique<Q4TimingRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropPhase=phase;
  assert(image->Execute() && !image->Matches(data.expected));numeric++;
 }
 printf("PASS_Q4_TIMING_LINKED_FAULTS refused%u numericalMismatches%u noIOAfterPark1;SYNTHETIC_NO_HARDWARE_AUTHORITY\n",refused,numeric);
}
