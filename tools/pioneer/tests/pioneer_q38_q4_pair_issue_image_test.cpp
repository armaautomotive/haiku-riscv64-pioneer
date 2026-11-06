/* SPDX-License-Identifier: MIT */
#define PIONEER_Q4_KERNEL_TIMING_MACHINE_ONLY
#include "pioneer_q38_q4_kernel_timing_image_test.cpp"
struct TracedQ4:Q4TimingRV32 {
 std::vector<unsigned> issued;
 TracedQ4(const char* folder,unsigned width,bool paired):Q4TimingRV32(folder,width,paired){}
 void Instruction(unsigned word)override{issued.push_back(word);Q4TimingRV32::Instruction(word);}
};
static void Park(Q4TimingRV32& image){
 assert(image.pc==0x70058 && !image.stores && !image.timerStores && !image.native.outputReads);
 const unsigned reads=image.bus.reads,writes=image.bus.writes,math=image.native.matrixOps,issues=image.bus.issues;
 for(unsigned i=0;i<2048;i++){image.TimingStep();image.Progress();}
 assert(image.pc==0x70058 && image.bus.reads==reads && image.bus.writes==writes
  && image.bus.issues==issues && image.native.matrixOps==math);
}
int main(int argc,char** argv){
 assert(argc==4);static_cast<void>(&NoMore);setvbuf(stdout,nullptr,_IOLBF,0);
 const unsigned k=unsigned(strtoul(argv[3],nullptr,10));assert(SL::Width(k));CompactStreamData data(k,0);
 auto old=std::make_unique<TracedQ4>(argv[1],k,false);
 auto next=std::make_unique<TracedQ4>(argv[2],k,true);
 data.Stage(old->bus);data.Stage(next->bus);
 assert(old->Execute() && next->Execute() && old->Matches(data.expected) && next->Matches(data.expected));
 if(old->issued!=next->issued){
  size_t at=0;while(at<old->issued.size() && at<next->issued.size() && old->issued[at]==next->issued[at])at++;
  fprintf(stderr,"PAIRED_WORD_STREAM_DIFFERENT first%zu oldCount%zu newCount%zu\n",at,old->issued.size(),next->issued.size());
 }
 fprintf(stdout,"PAIRED_RV32_OBSERVED width%u oldSteps%u pairedSteps%u\n",k,old->steps,next->steps);
 assert(old->issued==next->issued);
 assert(next->steps<old->steps);
 printf("PASS_PAIRED_ACTUAL_RV32 width%u sameTensixWords%zu oldSteps%u pairedSteps%u oldPort%u pairedPort%u oldStack%u pairedStack%u;COUNTS_NOT_CARD_CYCLES_OR_TPS\n",
  k,next->issued.size(),old->steps,next->steps,old->portWrites,next->portWrites,0x70000-old->minSP,0x70000-next->minSP);
 for(auto mode:{Q4TimingRV32::Unstable,Q4TimingRV32::Backward,Q4TimingRV32::Zero}){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);image->clock=mode;data.Stage(image->bus);
  assert(image->Execute() && image->Matches(data.expected));
 }
 unsigned refused=0,numeric=0;
 for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);data.Stage(image->bus);image->bus.fault=fault;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned field=0;field<8;field++){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);data.Stage(image->bus);
  const unsigned value=image->ReadL1(SL::Frame+field*4);
  image->WriteL1(SL::Frame+field*4,field<4?value^1:field==4?0:1);
  assert(!image->Execute() && !image->clockReads && !image->bus.reads && !image->bus.writes);Park(*image);refused++;
 }
 for(unsigned config:{72U,112U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);data.Stage(image->bus);image->dropConfig=config;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned ack:{1U,100U}){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);data.Stage(image->bus);image->dropAckAt=ack;
  assert(!image->Execute());Park(*image);refused++;
 }
 for(unsigned phase=0;phase<4;phase++){
  auto image=std::make_unique<Q4TimingRV32>(argv[2],k,true);data.Stage(image->bus);image->native.dropPhase=phase;
  assert(image->Execute() && !image->Matches(data.expected));numeric++;
 }
 printf("PASS_PAIRED_FAULTS width%u refused%u numericalMismatches%u unavailableClocks3 noIOAfterPark1;NO_NATIVE_AUTHORITY\n",k,refused,numeric);
}
