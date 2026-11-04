/* SPDX-License-Identifier: MIT */
// PRIVATE compiled data/actual RV32. No native IO or physical execution.
#define PIONEER_NATIVE_MIXED_STREAM_LEASE_ONLY
#include "pioneer_native_mixed_fpu_stream_lease_test.cpp"
#include "PioneerNativeMixedFpuStreamGeneratedImage.h"
#include "PioneerNativeMixedFpuStreamData.h"
#include "PioneerNativeMixedFpuStreamMetadata.h"
int main(int argc,char** argv) {
	assert(argc==3);static_cast<void>(&NoMore);static_cast<void>(&NoMoreLease);
	unsigned bands=0;
	for(unsigned s=0;s<17;s++)if(NV::Type(s)==FixtureType) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],s);auto p=io->Baseline();io->proof=&p;
		assert((l->Run<NLeaseIO,PioneerNativeMixedFpuStreamGeneratedImage,PioneerNativeMixedFpuStreamData>(*io,p,s)) && io->executed);
		const auto& r=l->Result();const auto* g=PioneerNativeMixedFpuStreamMetadata::Golden(s);assert(g);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==g[i%32]);
		assert((l->End<NLeaseIO,PioneerNativeMixedFpuStreamGeneratedImage,PioneerNativeMixedFpuStreamData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED
			&& r.phase==10 && r.compute.held && r.compute.l1_restored && r.compute.tlb_restored && r.gddr_restored && r.niu_restored && r.clock_restored);
		assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && !io->clock && !io->fifo && !io->retained && io->resets==4);
		for(unsigned a=0;a<2;a++)for(unsigned i=0;i<3;i++)assert(io->tlb[a][i]==a*3+i+1);for(unsigned v:io->vm.bus.command)assert(!v);
		const auto n=io->operations;
		assert((!l->Run<NLeaseIO,PioneerNativeMixedFpuStreamGeneratedImage,PioneerNativeMixedFpuStreamData>(*io,p,s)
			&& !l->End<NLeaseIO,PioneerNativeMixedFpuStreamGeneratedImage,PioneerNativeMixedFpuStreamData>(*io)) && io->operations==n);
		bands++;
		printf("PASS PRIVATE actual mixed87 image+header lease sample%u type%u tensor%u first%u rows%u K%u:all64 exact,whole107632B L1/%uB selected GDDR/NIU/FIFO/TLB/clock restored;NO native IO/hardware/full-model quality/speed proof\n",
			s,FixtureType,NV::Tensor(s),NV::First(s),NV::RealRows(s),NV::Width(s),NL::DramWords(s)*4);fflush(stdout);
	}
	assert(bands==(FixtureType==14?6:11));
	printf("PASS PRIVATE actual mixed87 type%u compiled data/image/RV32 lease:%u exact words/%u bands;NOT physical execution/full-model quality/performance authority\n",FixtureType,bands*64,bands);
}
