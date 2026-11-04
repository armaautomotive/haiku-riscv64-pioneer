/* SPDX-License-Identifier: MIT */
// PRIVATE real-data co-simulation of actual driver staging. No native IO.
#define PIONEER_NATIVE_Q4_FPU_LEASE_TEST_ONLY
#include "pioneer_native_q4_fpu_lease_test.cpp"
#include "PioneerNativeQ4FpuGeneratedImage.h"
#include "PioneerNativeQ4FpuData.h"
#include "PioneerNativeQ4FpuMetadata.h"
int main(int argc,char** argv) {
	assert(argc==2);static_cast<void>(&NoMore);
	for(unsigned sample=0;sample<3;sample++) {
		auto lease=std::make_unique<Lease>();auto io=std::make_unique<LeaseIO>(*lease,argv[1]);auto proof=io->Baseline();io->proof=&proof;
		assert((lease->Run<LeaseIO,PioneerNativeQ4FpuGeneratedImage,PioneerNativeQ4FpuData>(*io,proof,sample)) && io->executed);
		const auto& r=lease->Result();const auto* golden=PioneerNativeQ4FpuMetadata::Golden(sample);assert(golden);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==golden[i%32]);
		assert((lease->End<LeaseIO,PioneerNativeQ4FpuGeneratedImage,PioneerNativeQ4FpuData>(*io))
			&& r.compute.state==PIONEER_NATIVE_BFP_CLOSED && r.saved_l1==12372 && r.compute.l1_restored
			&& r.compute.tlb_restored && r.niu_restored && r.clock_restored && io->resets==4
			&& !io->clock && !io->retained && io->vm.native.l1==io->original);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)assert(io->tlb[s][i]==s*3+i+1);
		const auto before=io->operations;
		assert((!lease->Run<LeaseIO,PioneerNativeQ4FpuGeneratedImage,PioneerNativeQ4FpuData>(*io,proof,sample)
			&& !lease->End<LeaseIO,PioneerNativeQ4FpuGeneratedImage,PioneerNativeQ4FpuData>(*io)) && before==io->operations);
		printf("PASS PRIVATE real-header/actual-RV32 lease sample%u tensor7 first%u kblock%u all64 words exact;whole49488B/TLB/unchanged NIU/clock restored;NO native-IO/hardware/quality/speed proof\n",
			sample,PioneerNativeQ4FpuMetadata::First(sample),PioneerNativeQ4FpuMetadata::Block(sample));
	}
	puts("PASS PRIVATE actual compiled diagnostic85 image+header staging/co-simulation192 exact words;NOT physical execution,full-K/model quality or performance authority");
}
