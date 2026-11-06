/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
int main(int argc,char** argv){
 assert(argc==2);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 const std::string folder=argv[1];const auto code=Read((folder+"/text.bin").c_str());
 NImage::code[0]=NImage::code[1]=code;NImage::Boot=::Word(Read((folder+"/boot.bin").c_str()),0);assert(code.size()==NV::ImageBytes);
 for(unsigned s=0;s<9;s++)NData::samples.emplace_back(NV::Width(s),s);
 NData::Prepare115();
 unsigned closed=0;
 for(unsigned sample:{0U,6U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],sample);auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,sample)) && io->executed);
  const auto& r=lease->Result();const auto& data=NData::samples[sample];std::array<unsigned,32> expected{};
  for(const auto& b:data.blocks)for(unsigned g=0;g<8;g++)assert(PioneerQ38PreparedBf16Q4Reference::Group(b.original.data(),b.input.data(),g,expected.data()));
  for(unsigned row=0;row<32;row++)assert(r.compute.output[row]==data.expected[row] && r.compute.output[32+row]==expected[row]);
  assert(r.saved_l1==37136 && r.saved_gddr==NV::Width(sample)/256*2688 && r.requests==NV::Width(sample)/256*29 && r.flits==NV::Width(sample)/256*176);
  assert((lease->End<NLeaseIO,NImage,NData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED);
  assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo);
  NoMoreLease(*lease,*io);closed++;
 }
 assert(closed==2);unsigned refused=0;
 auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);auto baseline=io->Baseline();io->proof=&baseline;
 assert((lease->Run<NLeaseIO,NImage,NData>(*io,baseline,0)));
 const unsigned l1[]={0x87ff0,0x88800,0x8887c,0x8a000,0x8a040,0x8a080,0x8a100,0x8a180,
  0x8bfc0,0x8c000,0x8c3fc,0x9bfc0,0x9c000,0x9c3fc,0x90000,0x90100,0x911fc,
  0xa0000,0xa0100,0xa11fc,0x96000,0x961fc,0xa6000,0xa61fc,0x913fc,0xa13fc,
  0x94000,0xa4000,0x73000,0x98100};
 const unsigned dram[]={0x04000000,0x04000000+NL::WeightBytes(5120)-4,0x04080000,
  0x04080000+NL::PreparedBytes(5120)-4,0x04100000,0x04100000+NL::InputBytes(5120)-4};
 for(unsigned fault=0;fault<36;fault++){
  const pid_t pid=fork();assert(pid>=0);
  if(!pid){
   if(fault<30){const unsigned a=l1[fault];io->vm.WriteL1(a,io->vm.ReadL1(a)^1);}
   else io->vm.bus.dram[dram[fault-30]-NL::DramWeight]^=1;
   assert((!lease->End<NLeaseIO,NImage,NData>(*io)) && io->retained && io->retains==1
    && lease->Result().compute.cold && lease->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN);
   NoMoreLease(*lease,*io);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 }
 assert((lease->End<NLeaseIO,NImage,NData>(*io)) && io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained);
 NoMoreLease(*lease,*io);
 for(unsigned k:{5120U,17408U}){
  std::vector<unsigned> addresses;
  for(unsigned i=0;i<NL::DramWords(k);i++){const unsigned a=NL::DramAddress(k,i);assert(NL::DramContains(k,a));addresses.push_back(a);}
  assert(std::is_sorted(addresses.begin(),addresses.end()) && std::adjacent_find(addresses.begin(),addresses.end())==addresses.end()
   && !NL::DramContains(k,0x04060000) && !NL::DramContains(k,0x040f0000) && NL::DramAddress(k,NL::DramWords(k))==UINT32_MAX);
 }
 assert(refused==36);
 printf("PASS_PREPARED_BF16_LEASE bothWidths128Exact actualCompiledRV32_SourceTransfers_full148544B_L1_exactRawPreparedInputGDDR_NIU_TLB_FIFO_CLOCK_RESTORED;mutationRefusals%u_neverIOAfterRetain;SOFTWARE_NOT_NATIVE_HARDWARE_OR_TPS\n",refused);
}
