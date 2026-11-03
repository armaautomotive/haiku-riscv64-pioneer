/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP8_PLAN_H
#define PIONEER_Q38_NATIVE_BFP8_PLAN_H
#include "PioneerQ38NativeBfp8WeightTiles.h"
#include <map>
// PURE CPU DATA PROPOSAL, not physical allocation/route/range authority.
// No installed owner accepts this layout. Caller-supplied arena size is a
// HYPOTHESIS to check, never evidence of usable card memory. Existing3GiB
// bank capacity is NOT changed here. Original embeddings remain CPU-resident.
struct PioneerQ38NativeBfp8Plan {
	enum Kind {Weight=0,KV=1,Delta=2,Conv=3,Workspace=4};
	enum Result {Proposed=0,BadShape=1,InsufficientCapacity=2};
	struct Part {unsigned kind,layer,tensor,bank;uint64_t first,rows,rowBytes,offset,bytes,allocated;};
	struct Proposal {
		std::vector<Part> parts;std::array<uint64_t,7> used{};
		uint64_t capacity=0,weights=0,state=0,workspace=0;
		unsigned context=0;bool doubleDelta=false;
	};
private:
	static bool Stripe(Proposal& out,unsigned kind,unsigned layer,unsigned tensor,uint64_t rows,uint64_t rowBytes) {
		if(!rows || !rowBytes || rows>UINT64_MAX/rowBytes)return false;
		std::array<unsigned,7> banks{{0,1,2,3,4,5,6}};
		std::sort(banks.begin(),banks.end(),[&](unsigned a,unsigned b){return out.used[a]<out.used[b] || (out.used[a]==out.used[b] && a<b);});
		const unsigned count=rows<7?1:7;uint64_t first=0;
		for(unsigned p=0;p<count;p++) {
			const unsigned bank=banks[p];const uint64_t take=count==1?rows:rows/7+unsigned(p<rows%7);
			const uint64_t bytes=take*rowBytes;if(bytes>UINT64_MAX-4095)return false;
			const uint64_t allocated=(bytes+4095)&~UINT64_C(4095);
			if(out.used[bank]>out.capacity || allocated>out.capacity-out.used[bank])return false;
			out.parts.push_back({kind,layer,tensor,bank,first,take,rowBytes,out.used[bank],bytes,allocated});
			out.used[bank]+=allocated;first+=take;
			if(kind==Weight)out.weights+=bytes;else if(kind==Workspace)out.workspace+=bytes;else out.state+=bytes;
		}
		return first==rows;
	}
public:
	static Result Compute(const std::vector<PioneerQ38Tensor>& tensors,unsigned context,bool twoDelta,
		uint64_t hypotheticalCapacity,Proposal& output) {
		// Same64MiB base and32-bit address envelope are DATA constraints only.
		if(tensors.size()!=851 || !context || context>32768 || !hypotheticalCapacity || (hypotheticalCapacity&4095)
			|| hypotheticalCapacity>UINT64_C(0x100000000)-0x04000000)return BadShape;
		std::map<std::string,unsigned> names;
		for(unsigned i=0;i<tensors.size();i++)if(!names.emplace(tensors[i].name,i).second)return BadShape;
		Proposal next;next.capacity=hypotheticalCapacity;next.context=context;next.doubleDelta=twoDelta;
		for(unsigned ordinal=0;ordinal<=64;ordinal++) {
			const unsigned layer=ordinal?ordinal-1:64;
			for(unsigned role=0;;role++) {
				PioneerQ38Tensor expected;if(!PioneerQ38Expected(layer,role,expected))break;
				auto found=names.find(expected.name);if(found==names.end())return BadShape;
				const auto& tensor=tensors[found->second];
				if(tensor.type!=expected.type || tensor.rank!=expected.rank || tensor.inputs!=expected.inputs
					|| tensor.outputs!=expected.outputs || tensor.bytes!=expected.bytes)return BadShape;
				if(tensor.name=="token_embd.weight")continue;
				PioneerQ38NativeBfp8WeightTiles::Layout layout;
				if(tensor.type==0) {
					if(!Stripe(next,Weight,layer,found->second,1,tensor.bytes))return InsufficientCapacity;
				} else {
					if(!PioneerQ38NativeBfp8WeightTiles::Describe(tensor,layout))return BadShape;
					if(!Stripe(next,Weight,layer,found->second,layout.rowTiles,layout.columnTiles*1088))return InsufficientCapacity;
				}
			}
		}
		for(unsigned layer=0;layer<64;layer++) {
			if(layer%4==3) {
				if(!Stripe(next,KV,layer,851,8,uint64_t(context)*256*2))return InsufficientCapacity;
			} else {
				for(unsigned slot=0;slot<(twoDelta?2U:1U);slot++)
					if(!Stripe(next,Delta,layer,851,48,128*128*4))return InsufficientCapacity;
				if(!Stripe(next,Conv,layer,851,10240,3*4))return InsufficientCapacity;
			}
		}
		for(unsigned bank=0;bank<7;bank++) {
			const uint64_t bytes=64*1024*1024;
			if(next.used[bank]>next.capacity || bytes>next.capacity-next.used[bank])return InsufficientCapacity;
			next.parts.push_back({Workspace,64,851,bank,0,1,bytes,next.used[bank],bytes,bytes});next.used[bank]+=bytes;next.workspace+=bytes;
		}
		output=std::move(next);return Proposed;
	}
};
#endif
