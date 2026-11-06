/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_FACTORED_Q4_COMPARE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include "PioneerNativeCompactQ4StreamGeneratedImage.h"
#include "PioneerNativeCompactQ4StreamData.h"
#include "../PioneerNativeCompactQ4StreamSource.h"
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 PioneerNativeCompactQ4StreamSource source;std::string error;
 assert(source.Prepare(argv[2],error) && source.Ready());
 unsigned words=0;
 for(unsigned sample:{0U,6U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],sample);
  auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io,baseline,sample)));
  const auto& r=lease->Result();const auto* old=PioneerNativeCompactQ4StreamMetadata::Golden(sample);const auto* fresh=source.FactoredGolden(sample);
  assert(old && fresh);
  for(unsigned row=0;row<32;row++){assert(r.compute.output[row]==old[row] && r.compute.output[32+row]==fresh[row]);words+=2;}
  assert((lease->End<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED);
  assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo && source.Ready());
 }
 assert(words==128);
 puts("PASS_PRIVATE_FACTORED_COMPARE actualSameOpenOriginalSHA_all9Payloads_freshOldNewRefs actualCompiledLease_bothWidths128Exact fullL1_GDDR_NIU_TLB_FIFO_CLOCK_Restoration;NOT_HARDWARE_OR_MODEL_TPS");
}
