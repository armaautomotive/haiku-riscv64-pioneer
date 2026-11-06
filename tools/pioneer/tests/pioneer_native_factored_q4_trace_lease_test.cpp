/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_FACTORED_Q4_TRACE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include "../PioneerQ38Q4FactoredTraceReference.h"
int main(int argc,char** argv){
 assert(argc==2);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 const std::string folder=argv[1];const auto code=Read((folder+"/text.bin").c_str()),boot=Read((folder+"/boot.bin").c_str());
 assert(code.size()==NV::ImageBytes && NV::Samples==1 && NV::Role==112);NImage::code[0]=code;NImage::Boot=::Word(boot,0);
 NData::samples.emplace_back(5120,0);
 {
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);
  auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)) && io->executed);
  PioneerQ38Q4FactoredTraceReference reference;
  for(unsigned block=0;block<20;block++){const auto& b=NData::samples[0].blocks[block];assert(reference.Block(b.original.data(),b.input.data(),block));}
  unsigned expected[64];assert(reference.Export(expected));const auto& r=lease->Result();
  for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==expected[i]);
  for(unsigned v:r.timing)assert(!v);
  assert(r.saved_l1==37136 && r.saved_gddr==25600 && (lease->End<NLeaseIO,NImage,NData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED);
  assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo);NoMoreLease(*lease,*io);
 }
 unsigned refused=0;
 for(unsigned sample=1;sample<9;sample++){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);auto baseline=io->Baseline();io->proof=&baseline;
  assert((!lease->Run<NLeaseIO,NImage,NData>(*io,baseline,sample)) && !io->operations && !NV::Width(sample) && !NV::Count(sample));refused++;
 }
 for(unsigned address:{0x73000U,0x88000U,0x88040U,0x8843cU,0x88800U,0x88840U,0x88c3cU,0x98000U,0x98080U,0x980d0U,0x980e0U,0x98100U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)));io->vm.WriteL1(address,io->vm.ReadL1(address)^1);
  assert((!lease->End<NLeaseIO,NImage,NData>(*io)) && io->retained && io->retains==1);NoMoreLease(*lease,*io);refused++;
 }
 assert(refused==20);
 puts("PASS_TRACE112_LEASE compiledK5120 exactFinal32_trace32 auxZero16 full148544B_L1_GDDR_NIU_TLB_FIFO_CLOCK_restore;8unsupportedBandsZeroIO_12newDomainMutationsRetained;LOCAL_NOT_HARDWARE_AUTHORITY");
}
