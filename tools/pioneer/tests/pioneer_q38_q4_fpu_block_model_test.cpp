/* SPDX-License-Identifier: MIT */
// PRIVATE original-model READ ONLY. No private tensor/weight/result bytes out.
#define PIONEER_Q4_FPU_BLOCK_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_block_image_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
int main(int argc,char** argv)
{
	assert(argc==3);FILE* file=fopen(argv[2],"rb");assert(file);PioneerQwen38Gguf source;assert(source.Open(file));
	PioneerModelWeightCache::Digest digest;std::string error;
	assert(PioneerModelWeightCache::HashFile(fileno(file),PioneerQwen38Gguf::FullBytes,digest,error));
	assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	assert(source.Stable());uint64_t epoch=source.Epoch();unsigned cases=0;
	for(const char* name:{"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"}) {
		unsigned tensor=UINT32_MAX;for(unsigned i=0;i<source.Tensors().size();i++)if(source.Tensors()[i].name==name){tensor=i;break;}
		assert(tensor!=UINT32_MAX);const auto& t=source.Tensors()[tensor];assert(t.type==12 && (t.inputs==5120 || t.inputs==17408) && !(t.outputs%32));
		unsigned blocks=unsigned(t.inputs/256),rowBytes=blocks*144;
		for(unsigned first:{0U,unsigned((t.outputs/2)&~uint64_t(31)),unsigned(t.outputs)-32})
		for(unsigned block:{0U,blocks/2,blocks-1}) {
			Q4PipelineFixture data(0);
			for(unsigned row=0;row<32;row++)assert(source.ReadPacked(tensor,uint64_t(first+row)*rowBytes+block*144,144,data.original.data()+row*144,144));
			assert(source.Stable() && source.Epoch()==epoch);data.Refresh();
			Q4FpuBlockRV32 image(argv[1]);image.native.Stage(data.prepared,data.input);image.native.delay[0]=100003;image.native.delay[1]=7;
			assert(image.Execute() && image.Matches(data.expected));cases++;
			printf("PASS_PRIVATE_ACTUAL_Q4_FPU_BLOCK tensor%u width%u first%u kblock%u two_jobs/all64_words exact;diagnostic BF16 input,NOT real tokens/quality/speed\n",tensor,unsigned(t.inputs),first,block);fflush(stdout);
		}
	}
	assert(cases==27 && source.Stable() && source.Epoch()==epoch);assert(!fclose(file));
	printf("PASS_PRIVATE_ACTUAL_Q4_FPU_PIPELINE cases%u jobs54 result_words1728 weights221184,same-open full original SHA/epoch;NO source/converted weights/results printed/saved,NO device/native/model-quality/speed proof\n",cases);
}
