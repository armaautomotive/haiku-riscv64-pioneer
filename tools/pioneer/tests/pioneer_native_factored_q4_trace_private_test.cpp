/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_FACTORED_Q4_TRACE 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING 1
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include "PioneerNativeCompactQ4StreamGeneratedImage.h"
#include "PioneerNativeCompactQ4StreamData.h"
#include "../PioneerNativeCompactQ4StreamSource.h"
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
 PioneerNativeCompactQ4StreamSource source;std::string error;unsigned expected[64];
 assert(!source.TraceGolden(expected) && source.Prepare(argv[2],error) && source.TraceGolden(expected));
 auto lease=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*lease,argv[1],argv[1],0);
 auto baseline=io->Baseline();io->proof=&baseline;
 assert((lease->Run<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io,baseline,0)));
 const auto& result=lease->Result();for(unsigned i=0;i<64;i++)assert(result.compute.output[i]==expected[i]);
 assert(expected[0]==0x3f3b54c5 && source.Ready());
 assert((lease->End<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io)) && result.compute.state==PIONEER_NATIVE_BFP_CLOSED);
 assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->retained && !io->clock && !io->fifo && source.Ready());
 puts("PASS_PRIVATE_TRACE112 sameOpenOriginalSHA_all9Payloads_fixedSample0_freshIndependentStageReference64 actualCompiledNativeLease_completeRestoration;NOT_HARDWARE_QUALITY_OR_TPS");
}
