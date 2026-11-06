/* SPDX-License-Identifier: MIT */
// PRIVATE compiled source/actual RV32 staging.No native IO/hardware execution.
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
#include "pioneer_native_compact_q4_stream_lease_test.cpp"
#include "PioneerNativeCompactQ4StreamGeneratedImage.h"
#include "PioneerNativeCompactQ4StreamData.h"
#include "PioneerNativeCompactQ4StreamMetadata.h"
int main(int argc,char** argv) {
	assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
	for(unsigned s=0;s<9;s++) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],s);auto p=io->Baseline();io->proof=&p;
		assert((l->Run<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io,p,s)) && io->executed);
		const auto& r=l->Result();const auto* g=PioneerNativeCompactQ4StreamMetadata::Golden(s);assert(g);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==g[i%32]);
		assert((l->End<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED
			&& r.phase==10 && r.compute.held && r.compute.l1_restored && r.compute.tlb_restored && r.gddr_restored && r.niu_restored && r.clock_restored);
		assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->clock && !io->fifo && !io->retained && io->resets==4);
		for(unsigned a=0;a<2;a++)for(unsigned i=0;i<3;i++)assert(io->tlb[a][i]==a*3+i+1);for(unsigned v:io->vm.bus.command)assert(!v);
		const auto n=io->operations;
		assert((!l->Run<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io,p,s)
			&& !l->End<NLeaseIO,PioneerNativeCompactQ4StreamGeneratedImage,PioneerNativeCompactQ4StreamData>(*io)) && io->operations==n);
		printf("PASS PRIVATE actual compact full-K%u image+header lease sample%u tensor%u first%u K%u:all64 exact,whole107632B L1/%uB GDDR/NIU/FIFO/TLB/clock restored;NO native IO/hardware/full-model quality/speed proof\n",
			NV::Role,s,NV::Tensor(s),NV::First(s),NV::Width(s),NL::DramWords(NV::Width(s))*4);fflush(stdout);
	}
	puts("PASS PRIVATE actual compiled compact full-K104 data/image/RV32 lease576 exact words over9 bands;NOT physical execution/full-model quality/performance authority");
}
