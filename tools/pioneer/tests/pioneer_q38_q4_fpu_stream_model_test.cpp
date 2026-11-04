/* SPDX-License-Identifier: MIT */
// PRIVATE original model read-only. No original/derived weight bytes out.
#define PIONEER_Q4_STREAM_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_stream_image_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
int main(int argc,char** argv) {
	assert(argc==4);static_cast<void>(&NoMore);FILE* f=fopen(argv[3],"rb");assert(f);PioneerQwen38Gguf source;assert(source.Open(f));
	PioneerModelWeightCache::Digest digest;std::string error;
	assert(PioneerModelWeightCache::HashFile(fileno(f),PioneerQwen38Gguf::FullBytes,digest,error));
	assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	assert(source.Stable());uint64_t epoch=source.Epoch();unsigned cases=0;
	for(const char* name:{"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"}) {
		unsigned index=~0U;for(unsigned i=0;i<source.Tensors().size();i++)if(source.Tensors()[i].name==name){index=i;break;}
		assert(index!=~0U);const auto& t=source.Tensors()[index];unsigned k=unsigned(t.inputs);assert(t.type==12 && SL::Width(k) && !(t.outputs%32));
		for(unsigned first:{0U,unsigned((t.outputs/2)&~uint64_t(31)),unsigned(t.outputs)-32}) {
			StreamData data(k,0);for(unsigned block=0;block<k/256;block++)for(unsigned row=0;row<32;row++)
				assert(source.ReadPacked(index,(uint64_t(first+row)*(k/256)+block)*144,144,data.blocks[block].original.data()+row*144,144));
			assert(source.Stable() && source.Epoch()==epoch);data.Refresh();
			auto image=std::make_unique<StreamRV32>(k==5120?argv[1]:argv[2],k);data.Stage(image->bus);
			assert(image->Execute() && image->Matches(data.expected));cases++;
			printf("PASS_PRIVATE_ACTUAL_Q4_STREAM tensor%u width%u first%u two_complete_jobs/all64_words exact,persistent accumulation,actual NoC/shared L1/compiled RV32;diagnostic BF16 inputs,NOT native hardware/real tokens/quality/speed\n",index,k,first);fflush(stdout);
		}
	}
	assert(cases==9 && source.Stable() && source.Epoch()==epoch && !fclose(f));
	puts("PASS_PRIVATE_ACTUAL_Q4_FULL_K_STREAM cases9 jobs18 exact_words576 original_weights2654208,same-open full original SHA/epoch;NO learned bytes saved/printed,NO native owner/hardware/full-model-quality/speed proof");
}
