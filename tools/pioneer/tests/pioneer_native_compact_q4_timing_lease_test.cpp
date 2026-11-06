/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING) || PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING!=1
#error Explicit timed lease test required
#endif
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 for(unsigned w=0;w<2;w++){
  std::string d=argv[w+1];NImage::code[w]=Read((d+"/text.bin").c_str());assert(NImage::code[w].size()==NV::ImageBytes);
  NImage::Boot=::Word(Read((d+"/boot.bin").c_str()),0);
 }
 for(unsigned s=0;s<9;s++)NData::samples.emplace_back(NV::Width(s),s);
 unsigned good=0,bad=0;
 for(auto mode:{Q4TimingRV32::Monotonic,Q4TimingRV32::Unstable,Q4TimingRV32::Backward,Q4TimingRV32::Zero}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[2],0);
  auto baseline=io->Baseline();io->proof=&baseline;io->vm.clock=mode;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)));
  const auto& result=lease->Result();
  for(unsigned pass=0;pass<2;pass++)assert(PioneerQ38Q4KernelTiming::State(result.timing+pass*8,5120,pass)==(mode==Q4TimingRV32::Monotonic?1U:2U));
  for(unsigned i=0;i<64;i++)assert(result.compute.output[i]==NData::samples[0].expected[i%32]);
  assert((lease->End<NLeaseIO,NImage,NData>(*io)) && result.compute.state==PIONEER_NATIVE_BFP_CLOSED
   && io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && io->timingReads==32);
  NoMoreLease(*lease,*io);good++;
 }
 // Every raw word is captured and compared after HOLD before overwriting.
 for(unsigned word=0;word<16;word++){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[2],0);
  auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)));
  const unsigned address=PioneerQ38Q4KernelTiming::Address+word*4;
  io->vm.WriteL1(address,io->vm.ReadL1(address)^1);
  assert((!lease->End<NLeaseIO,NImage,NData>(*io)) && io->retained && io->retains==1);
  NoMoreLease(*lease,*io);bad++;
 }
 for(unsigned read=1;read<=32;read++){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[2],0);
  auto baseline=io->Baseline();io->proof=&baseline;io->dropTimingRead=read;
  const bool ran=lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0);
  if(read<=16)assert(!ran);else assert(ran && (!lease->End<NLeaseIO,NImage,NData>(*io)));
  assert(io->retained && io->retains==1 && lease->Result().compute.cold);NoMoreLease(*lease,*io);bad++;
 }
 printf("PASS_TIMED_Q4_ACTUAL_JOURNAL clocks%u timerMutationAndReadFaults%u fullRestoration1 unavailableClockStillCloses1 noIOAfterRetain1;SYNTHETIC_NO_NATIVE_HARDWARE_AUTHORITY\n",good,bad);
}
