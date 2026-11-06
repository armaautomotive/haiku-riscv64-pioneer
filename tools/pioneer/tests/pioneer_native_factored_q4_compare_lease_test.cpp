/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_FACTORED_Q4_COMPARE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 const std::string folder=argv[1];const auto code=Read((folder+"/text.bin").c_str()),boot=Read((folder+"/boot.bin").c_str());
 assert(code.size()==NV::ImageBytes);NImage::code[0]=NImage::code[1]=code;NImage::Boot=::Word(boot,0);
 for(unsigned s=0;s<9;s++)NData::samples.emplace_back(NV::Width(s),s);
 unsigned closed=0,failed=0;
 for(unsigned sample:{0U,6U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],sample);
  auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,sample)) && io->executed);
  std::array<uint32_t,32> expected{};const auto& data=NData::samples[sample];
  for(const auto& block:data.blocks)assert(PioneerQ38FactoredQ4Reference::Block(block.original.data(),block.input.data(),expected.data()));
  const auto& r=lease->Result();
  for(unsigned row=0;row<32;row++){assert(r.compute.output[row]==data.expected[row]);assert(r.compute.output[32+row]==expected[row]);}
  assert(r.saved_l1==37136 && (lease->End<NLeaseIO,NImage,NData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED);
  assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo);
  NoMoreLease(*lease,*io);closed++;
 }
 // New code region,temporary native tiles and old/new outputs must ALL be
 // captured before restoration. Never exempt the final packed-code tile.
 for(unsigned address:{0x73000U,0x88000U,0x88040U,0x8843cU,0x88800U,0x88840U,0x88c3cU,0x98080U,0x98100U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);
  auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)));
  io->vm.WriteL1(address,io->vm.ReadL1(address)^1);
  assert((!lease->End<NLeaseIO,NImage,NData>(*io)) && io->retained && io->retains==1);NoMoreLease(*lease,*io);failed++;
 }
 assert(closed==2 && failed==9);
 puts("PASS_FACTORED_COMPARE_LEASE fullK5120_17408 actualCompiledBothRecipes independentOutputs whole148544B_L1_GDDR_NIU_TLB_FIFO_CLOCK_restore;9newDomainMutationsRetained;SOFTWARE_NOT_NATIVE_HARDWARE_AUTHORITY");
}
