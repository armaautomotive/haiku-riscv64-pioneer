/* SPDX-License-Identifier: MIT */
#define PIONEER_PARALLEL_DECODER_MACHINE_ONLY
#include "pioneer_parallel_decoder_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderStageProfile.h"
struct ProfileDecoder:ParallelDecoder {
	unsigned timerReads=0,traceWrites=0;
	ProfileDecoder(const char* path,const DecoderSfpuImage& image,bool nz):ParallelDecoder(path,image,nz,true) {}
	uint32_t Load(uint32_t a) override {
		if (a==0xffb121f0) { timerReads++; return 0xfffffff8+steps; }
		if (Range(a,TT_DECODER_STAGE_L1,128)) return memory.at(a);
		return ParallelDecoder::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,TT_DECODER_STAGE_L1,128)) { traceWrites++; memory[a]=value; return; }
		ParallelDecoder::Store(a,value);
	}
	void VerifyProfile() {
		tt_decoder_stage_profile p={}; for (unsigned i=0;i<32;i++) reinterpret_cast<uint32_t*>(&p)[i]=memory.at(TT_DECODER_STAGE_L1+i*4);
		assert(PioneerDecoderStageValid(p,positions,positions) && timerReads==2*(196+113+113+28+28+28+1+1)*positions);
		for (unsigned i=0;i<32;i++) { auto broken=p;
			if (i>=4 && i<12) { reinterpret_cast<uint32_t*>(&broken)[i]=0; } else reinterpret_cast<uint32_t*>(&broken)[i]^=1;
			assert(!PioneerDecoderStageValid(broken,positions,positions));
		}
	}
};
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true; DecoderSfpuImage image(argv[2],false,true);
	{ ProfileDecoder m(argv[1],image,false); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); }
	{ ProfileDecoder m(argv[1],image,true); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); m.Next(); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); }
	puts("PASS OFFLINE actual profiled whole decoder:only documented clock-low READ added,exact immutable bytes/trace bounds/8 stage counts/wrapped timer/quality/counters/KV/sequences;NOT native timing qualification");
}
