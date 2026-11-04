/* SPDX-License-Identifier: MIT */
// PRIVATE original model READ ONLY. Does not print/save any weight data.
#include "pioneer_q38_q4_fpu_block_observer.h"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
int main(int argc,char** argv)
{
	assert(argc==2);FILE* file=fopen(argv[1],"rb");assert(file);
	PioneerQwen38Gguf source;assert(source.Open(file));
	PioneerModelWeightCache::Digest digest;std::string error;
	assert(PioneerModelWeightCache::HashFile(fileno(file),PioneerQwen38Gguf::FullBytes,digest,error));
	assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	assert(source.Stable());const uint64_t epoch=source.Epoch();uint64_t count=0;unsigned matrices=0,bands=0;
	// This model's K6144 projections are Q8/Q6, NOT Q4. Test the three
	// actual device FFN families,not the CPU-only token embedding or an
	// invented third Q4 width. Other quant formats need separate producers.
	const char* names[]={"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
	for(const char* name:names) {
		unsigned tensor=UINT32_MAX;
		for(unsigned i=0;i<source.Tensors().size();i++) {
			const auto& t=source.Tensors()[i];if(t.name==name){tensor=i;break;}
		}
		assert(tensor!=UINT32_MAX);const auto& t=source.Tensors()[tensor];
		assert(t.type==12 && (t.inputs==5120 || t.inputs==17408) && t.outputs>=32 && !(t.outputs%32));
		const unsigned width=unsigned(t.inputs),rowBytes=(width/256)*144;
		const unsigned first[]={0U,unsigned((t.outputs/2)&~uint64_t(31)),unsigned(t.outputs)-32};
		for(unsigned band=0;band<3;band++) {
			assert(first[band]+32<=t.outputs);const uint64_t before=count;
			for(unsigned block=0;block<width/256;block++) {
				uint8_t original[32][144];
				for(unsigned row=0;row<32;row++)assert(source.ReadPacked(tensor,uint64_t(first[band]+row)*rowBytes+block*144,144,original[row],144));
				CheckQ4FpuBlock(original,count);
			}
			assert(source.Stable() && source.Epoch()==epoch && count-before==uint64_t(width)*32);bands++;
			printf("PASS_PRIVATE_Q4_FPU_BAND tensor%u width%u first%u rows32 weights%llu exact original-decode/BF16-RNE;NO weight bytes printed,NO hardware/model-quality/speed proof\n",tensor,width,first[band],(unsigned long long)(count-before));fflush(stdout);
		}
		matrices++;
	}
	assert(matrices==3 && bands==9 && count==uint64_t(5120+5120+17408)*32*3);
	assert(source.Stable() && source.Epoch()==epoch);assert(!fclose(file));
	printf("PASS_PRIVATE_Q4_FPU_BINDING matrices%u bands%u weights%llu same-open full original SHA/epoch;NO card/Assistant/cache/driver changes or model-quality/speed qualification\n",matrices,bands,(unsigned long long)count);
}
