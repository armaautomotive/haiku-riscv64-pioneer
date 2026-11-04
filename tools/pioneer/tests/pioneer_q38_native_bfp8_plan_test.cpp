/* SPDX-License-Identifier: MIT */
// Synthetic exact-model METADATA only: no original model/IO/physical memory.
#include "../PioneerQ38NativeBfp8Plan.h"
#include <cassert>
#include <cstdio>
int main() {
	using P=PioneerQ38NativeBfp8Plan;std::vector<PioneerQ38Tensor> tensors;
	for(unsigned ordinal=0;ordinal<=64;ordinal++)for(unsigned role=0;;role++) {
		PioneerQ38Tensor tensor;if(!PioneerQ38Expected(ordinal?ordinal-1:64,role,tensor))break;tensors.push_back(tensor);
	}
	assert(tensors.size()==851);P::Proposal proposal;proposal.capacity=17;unsigned cases=0;
	assert(P::Compute(tensors,128,true,UINT64_C(3)*1024*1024*1024,proposal)==P::InsufficientCapacity && proposal.capacity==17 && proposal.parts.empty());cases++;
	const uint64_t candidate=UINT64_C(15)*1024*1024*1024/4;
	assert(P::Compute(tensors,128,true,candidate,proposal)==P::Proposed);cases++;
	assert(proposal.capacity==candidate && proposal.weights==UINT64_C(27242264576));
	assert(proposal.state==UINT64_C(316276736) && proposal.workspace==UINT64_C(469762048));
	std::array<uint64_t,7> end{};std::array<uint64_t,851> bytes{},rows{};
	uint64_t allocations=0,quantized=0,floats=0;unsigned workspaces=0;
	for(const auto& part:proposal.parts) {
		assert(part.bank<7 && part.rows && part.rowBytes && part.bytes==part.rows*part.rowBytes);
		assert(!(part.offset&4095) && !(part.allocated&4095) && part.bytes<=part.allocated && part.offset==end[part.bank]);
		assert(part.offset<=candidate && part.allocated<=candidate-part.offset);end[part.bank]+=part.allocated;allocations+=part.allocated;
		if(part.kind==P::Weight) {
			assert(part.tensor<851 && part.first==rows[part.tensor]);bytes[part.tensor]+=part.bytes;rows[part.tensor]+=part.rows;
		}else {assert(part.tensor==851);if(part.kind==P::Workspace)workspaces++;}
	}
	assert(end==proposal.used && workspaces==7);
	for(unsigned i=0;i<tensors.size();i++) {
		const auto& tensor=tensors[i];PioneerQ38NativeBfp8WeightTiles::Layout layout;
		if(tensor.name=="token_embd.weight"){assert(!bytes[i] && !rows[i]);continue;}
		if(tensor.type==0){assert(bytes[i]==tensor.bytes && rows[i]==1);floats+=bytes[i];}
		else {assert(PioneerQ38NativeBfp8WeightTiles::Describe(tensor,layout));assert(bytes[i]==layout.bytes && rows[i]==layout.rowTiles);quantized+=bytes[i];}
	}
	const auto oldUsed=proposal.used;const auto oldParts=proposal.parts.size();
	for(unsigned bad=0;bad<10;bad++) {
		auto changed=tensors;unsigned context=128;uint64_t cap=candidate;
		if(bad==0)changed.pop_back();
		if(bad==1)changed[1].name=changed[0].name;
		if(bad==2)changed[0].outputs++;
		if(bad==3)changed[0].bytes--;
		if(bad==4)context=0;
		if(bad==5)context=32769;
		if(bad==6)cap++;
		if(bad==7)cap=UINT64_C(4)*1024*1024*1024;
		if(bad==8)cap=UINT64_C(0x100000000)-P::Base; // OLD maximum included tile registers.
		if(bad==9)cap=P::MaxCapacity+4096;
		assert(P::Compute(changed,context,true,cap,proposal)==P::BadShape && proposal.used==oldUsed && proposal.parts.size()==oldParts);cases++;
	}
	assert(P::Compute(tensors,32768,true,candidate,proposal)==P::InsufficientCapacity && proposal.used==oldUsed);cases++;
	P::Proposal limit;assert(P::Compute(tensors,128,true,P::MaxCapacity,limit)==P::Proposed);cases++;
	assert(P::Base+limit.capacity==P::GddrEnd && P::GddrEnd==UINT64_C(0xff000000));
	for(const auto& part:limit.parts)assert(P::Base+part.offset+part.allocated<=P::GddrEnd);
	// CONDITIONAL traffic model,NOT a hardware benchmark/universal speed cap.
	// Rated p100a BW: https://docs.tenstorrent.com/aibs/blackhole/index.html .
	// Assume naive batch1 reads all device matrices per token;even optimistically
	// dedicating ALL180MiB SRAM to static weights cannot reach20 in this dataflow.
	assert(quantized==UINT64_C(27231682560) && floats==UINT64_C(10582016));
	const uint64_t bandwidth=UINT64_C(448000000000),sram=UINT64_C(180)*1024*1024;
	assert((quantized-sram)*20>bandwidth);cases++;
	printf("ANALYTICAL_ONLY native-BFP8 naive batch1 stream:%llu matrix bytes/token,448000000000B/s advertised;ideal%.6f tokens/s,optimistic all180MiB static SRAM%.6f;excludes compute/state/overhead,NOT measured speed or a universal algorithm bound;20 requires lower traffic/reuse\n",(unsigned long long)quantized,double(bandwidth)/double(quantized),double(bandwidth)/double(quantized-sram));
	std::reverse(tensors.begin(),tensors.end());P::Proposal reordered;
	assert(P::Compute(tensors,128,true,candidate,reordered)==P::Proposed && reordered.used==oldUsed && reordered.weights==proposal.weights);cases++;
	printf("PASS native-BFP8 DATA proposal:%u cases,all851 exact shapes,497 quantized matrices plus353 float weights,NO CPU embedding copy,128-token double-delta/KV/conv and7x64MiB workspace,disjoint page-aligned complete coverage;current3GiB/bank REFUSED,3.75GiB/bank HYPOTHESIS allocated%llu;NO physical capacity,driver range,allocation,GPU or speed authority\n",cases,(unsigned long long)allocations);
}
