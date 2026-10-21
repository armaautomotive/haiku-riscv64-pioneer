/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../PioneerQ38NativeQ8Data.h"
struct SourceModel {struct Tensor{unsigned type=8;};std::vector<Tensor> tensors={Tensor{}};const auto& Tensors()const{return tensors;}};
struct PioneerQwen38PackedPlan {
	struct Part{unsigned tensor=0;uint64_t bytes=576;};std::vector<Part> parts={Part{}};
	std::array<uint8_t,4096> data{};mutable unsigned reads=0;bool match=true,readOkay=true;
	bool MatchesSource(const SourceModel&)const{return match;}
	const auto& Parts()const{return parts;}
	bool ReadWeightPage(SourceModel&,unsigned fragment,unsigned page,uint8_t* out,unsigned capacity)const {
		reads++;if(!readOkay || fragment || page || capacity<4096)return false;
		std::memcpy(out,data.data(),4096);return true;
	}
};
struct Source {
	SourceModel model;unsigned calls=0,failOn=0;bool ready=true;
	bool Ready(){calls++;return ready && calls!=failOn;}
#include "pioneer_q38_native_q8_source_page.inc"
};
int main() {
	unsigned cases=0;std::array<uint8_t,4096> out;
	for(unsigned fault=0;fault<14;fault++) {
		Source source;PioneerQwen38PackedPlan plan;out.fill(0xdd);uint8_t* target=out.data();unsigned fragment=0,page=0,capacity=4096;
		if(fault==0)source.ready=false;if(fault==1)target=nullptr;if(fault==2)capacity=4095;if(fault==3)plan.match=false;
		if(fault==4)plan.readOkay=false;if(fault==5)source.failOn=2;if(fault==6)source.failOn=3;
		if(fault==7)plan.parts.clear();if(fault==8)plan.parts[0].tensor=1;if(fault==9)plan.parts[0].bytes=0;
		if(fault==10)plan.data[32]=128;if(fault==11)plan.data[576]=1;if(fault==12)fragment=1;if(fault==13)page=1;
		assert(!source.ReadWeightPage(plan,fragment,page,target,capacity));for(auto b:out)assert(b==0xdd);cases++;
	}
	for(unsigned type:{0U,8U,12U,14U}) {
		Source source;source.model.tensors[0].type=type;PioneerQwen38PackedPlan plan;
		plan.data[1]=128;for(unsigned i=32;i<544;i++)plan.data[i]=i%2?255:127;
		out.fill(0xdd);assert(source.ReadWeightPage(plan,0,0,out.data(),4096) && plan.reads==1);
		for(unsigned i=0;i<4096;i++)assert(out[i]==(type==8 && i>=32 && i<544 && (i%2)?129:plan.data[i]));
		assert(source.calls==unsigned(type==8?3:2));
	}
	printf("PASS %u extracted ACTUAL authenticated source-page refusal cases+all4 quant types;SM7 ONLY Q8,metadata/padding exact,fresh source before/after conversion,bounds/read/model/source failures BEFORE publication;NO_MODEL_OR_ACCELERATOR_IO\n",cases);
}
