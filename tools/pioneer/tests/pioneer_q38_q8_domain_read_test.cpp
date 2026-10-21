/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
struct InventorySource {
	struct Tensor{unsigned type=8,rank=2;};
	struct Model {
		std::vector<Tensor> tensors={Tensor{}};unsigned calls=0;bool fail=false;
		const auto& Tensors()const{return tensors;}
		bool ReadPacked(unsigned,uint64_t offset,unsigned bytes,void* out,unsigned capacity) {
			calls++;if(fail || offset>34*32768 || bytes>34*32768-offset || capacity<bytes)return false;
			auto* data=static_cast<uint8_t*>(out);for(unsigned i=0;i<bytes;i++)data[i]=uint8_t((offset+i)%251);return true;
		}
	} model;
	unsigned readyCalls=0;bool ready=true,changeAfterRead=false;
	bool Ready(){readyCalls++;return ready && !(changeAfterRead && model.calls);}
#include "pioneer_q38_q8_domain_read.inc"
};
int main() {
	std::vector<uint8_t> out(34*16384,0xdd);unsigned cases=0;
	for(unsigned fault=0;fault<9;fault++) {
		InventorySource source;unsigned tensor=0,bytes=34*16384,capacity=bytes;uint64_t offset=0;uint8_t* target=out.data();
		if(fault==0)source.ready=false;if(fault==1)tensor=1;if(fault==2)target=nullptr;
		if(fault==3)bytes=0;if(fault==4)bytes+=34;if(fault==5)capacity--;
		if(fault==6)offset=1;if(fault==7)bytes--;if(fault==8)offset=UINT64_MAX;
		assert(!source.ReadQ8DomainChunk(tensor,offset,bytes,target,capacity) && !source.model.calls);
		for(auto b:out)assert(b==0xdd);cases++;
	}
	for(unsigned fault=0;fault<4;fault++) {
		InventorySource source;
		if(fault==0)source.model.tensors[0].type=12;if(fault==1)source.model.tensors[0].rank=1;
		if(fault==2)source.model.fail=true;if(fault==3)source.changeAfterRead=true;
		assert(!source.ReadQ8DomainChunk(0,0,34,out.data(),unsigned(out.size())));
		assert(source.model.calls==unsigned(fault>=2));cases++;
	}
	InventorySource source;
	assert(source.ReadQ8DomainChunk(0,34*16384,34*16384,out.data(),unsigned(out.size())) && source.model.calls==1 && source.readyCalls==2);
	for(unsigned i=0;i<out.size();i++)assert(out[i]==uint8_t((34*16384+i)%251));
	printf("PASS %u extracted ACTUAL Q8 inventory read refusals+bounded positive chunk;fresh source before+after,alignment/type/rank/size/capacity/model-error/post-read-change;NO_IO_OR_ACCELERATOR\n",cases);
}
