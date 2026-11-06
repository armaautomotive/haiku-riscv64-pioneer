/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include "PioneerNativeCompactQ4StreamGeneratedImage.h"
#include "../PioneerNativePreparedBf16Q4Source.h"
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 PioneerNativePreparedBf16Q4Source source;std::string error;
 puts("PREPARED115_PRIVATE_AUTHENTICATING_SAME_ORIGINAL_FD_ALL9_RAW_PREPARED_INPUT_AND_BOTH_REFERENCES_NO_DEVICE");fflush(stdout);
 assert(source.Prepare(argv[2],error) && source.Ready());unsigned words=0;
 for(unsigned sample:{0U,6U}){
  auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],sample);auto baseline=io->Baseline();io->proof=&baseline;
  assert((lease->Run<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativePreparedBf16Q4Data>(*io,baseline,sample)) && io->executed);
  const auto& r=lease->Result();const auto* old=PioneerNativeCompactQ4StreamMetadata::Golden(sample);const auto* fresh=source.Golden(sample);
  assert(old && fresh);for(unsigned row=0;row<32;row++){assert(r.compute.output[row]==old[row] && r.compute.output[32+row]==fresh[row]);words+=2;}
  assert((lease->End<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativePreparedBf16Q4Data>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED);
  assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo && source.Ready());
  NoMoreLease(*lease,*io);
 }
 assert(words==128);
 puts("PASS_PRIVATE_PREPARED115_LEASE sameOpenOriginalSHA_all9RawPreparedInputBindings_freshBothReferences actualCompiledBothWidths128Exact WholeL1_ThreeGDDRSpans_NIU_TLB_FIFO_CLOCK_RESTORED;SOFTWARE_NOT_HARDWARE_OR_MODEL_TPS");
}
