/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_PACKED_PLAN_H
#define PIONEER_QWEN38_PACKED_PLAN_H
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38TiledReader.h"

// OFFLINE canonical packed model blueprint / bounded page reader. No route,
// allocation,upload,driver,execution or launch authority. In particular this
// MUST NOT be passed as coordinates to the installed bank-zero-only driver.
// Canonical semantic order,not untrusted GGUF descriptor order,sets layout.
enum { PIONEER_Q38_WEIGHT=0,PIONEER_Q38_DELTA_STATE=1,PIONEER_Q38_CONV_STATE=2,
	PIONEER_Q38_KV_STATE=3,PIONEER_Q38_WORKSPACE=4 };
struct PioneerQ38PackedPart {
	unsigned kind,layer,role,tensor,bank,firstRow,rows,rowBytes;
	uint32_t proposedAddress,bytes,allocated;
};
class PioneerQwen38PackedPlan {
	std::vector<PioneerQ38PackedPart> parts;
	std::vector<PioneerQ38PackedPart> empty;
	uint64_t used[7]={};
	bool ready=false;
	bool tiled=false;
	mutable PioneerQwen38TiledReader tiledReader;
	const PioneerQwen38Gguf* boundSource=NULL;
	uint64_t epoch=0;
	bool Stripe(unsigned kind,unsigned layer,unsigned role,unsigned tensor,unsigned rows,unsigned rowBytes,unsigned unit) {
		if (!rows || !rowBytes || !unit) return false;
		unsigned counts[7]={},chunks=rows/unit,tail=rows%unit,count=chunks<7?1:7;
		if (count==1) counts[0]=rows;
		else { for (unsigned b=0;b<7;b++) counts[b]=(chunks/7+unsigned(b<chunks%7))*unit; counts[6]+=tail; }
		unsigned banks[]={0,1,2,3,4,5,6};
		std::sort(banks,banks+7,[this](unsigned a,unsigned b){return used[a]<used[b] || (used[a]==used[b] && a<b);});
		unsigned first=0;
		for (unsigned p=0;p<count;p++) {
			unsigned bank=banks[p]; uint64_t bytes=uint64_t(counts[p])*rowBytes,allocated=(bytes+4095)&~UINT64_C(4095);
			if (!counts[p] || bytes>UINT32_MAX || allocated>Capacity-used[bank]) return false;
			PioneerQ38PackedPart part={kind,layer,role,tensor,bank,first,counts[p],rowBytes,
				uint32_t(Base+used[bank]),uint32_t(bytes),uint32_t(allocated)};
			parts.push_back(part); used[bank]+=allocated; first+=counts[p];
		}
		return first==rows;
	}
public:
	static const uint64_t Capacity=UINT64_C(3)*1024*1024*1024;
	static const uint32_t Base=0x04000000;
	bool Ready() const { return ready && boundSource && boundSource->Epoch()==epoch && boundSource->Stable(); }
	bool Tiled() const { return Ready() && tiled; }
	const std::vector<PioneerQ38PackedPart>& Parts() const { return Ready()?parts:empty; }
	uint64_t Used(unsigned bank) const { return Ready() && bank<7?used[bank]:0; }
	bool Build(const PioneerQwen38Gguf& source,unsigned context,bool tileMajor=false) {
		ready=false; boundSource=NULL; epoch=0; tiled=false; tiledReader.Reset(); parts.clear(); for (auto& bytes:used) bytes=0;
		if (!source.Stable() || !context || context>32768) return false;
		// Three global tensors first,then each exact dense/recurrent layer.
		for (unsigned ordinal=0;ordinal<=64;ordinal++) {
			unsigned layer=ordinal?ordinal-1:64;
			for (unsigned role=0;;role++) {
				PioneerQ38Tensor expected; if (!PioneerQ38Expected(layer,role,expected)) break;
				const auto* tensor=source.Find(expected.name.c_str()); if (!tensor) return false;
				unsigned index=unsigned(tensor-source.Tensors().data());
				unsigned rowBytes=unsigned(tensor->bytes/tensor->outputs);
				if (tileMajor && tensor->type!=0) {
					if (tensor->outputs%16) return false;
					rowBytes=PioneerQ38TiledRowBytes(tensor->type,tensor->inputs);
					if (!rowBytes) return false;
				}
				if (!Stripe(PIONEER_Q38_WEIGHT,layer,role,index,unsigned(tensor->outputs),rowBytes,64)) return false;
			}
		}
		for (unsigned layer=0;layer<64;layer++) {
			if (layer%4==3) {
				// Eight complete K/V heads. Device state will be HEAD-major;
				// reference attention helper's token-major view needs an adapter.
				if (!Stripe(PIONEER_Q38_KV_STATE,layer,0,851,8,context*256*2,1)) return false;
			} else {
				if (!Stripe(PIONEER_Q38_DELTA_STATE,layer,0,851,48,128*128*4,1)
					|| !Stripe(PIONEER_Q38_CONV_STATE,layer,0,851,10240,3*4,64)) return false;
			}
		}
		for (unsigned bank=0;bank<7;bank++) {
			const unsigned bytes=64*1024*1024;
			if (bytes>Capacity-used[bank]) return false;
			parts.push_back({PIONEER_Q38_WORKSPACE,64,0,851,bank,0,1,bytes,uint32_t(Base+used[bank]),bytes,bytes}); used[bank]+=bytes;
		}
		ready=source.Stable() && (!tileMajor || tiledReader.Bind(source));
		if (ready) { boundSource=&source; epoch=source.Epoch(); tiled=tileMajor; } return ready;
	}
	// Ordinary CPU buffer only. NEVER expose stale host bytes in last-page
	// padding. Transport must independently OWN/copy any future DMA staging;
	// this reader is NOT a transport and does not submit any device command.
	bool ReadWeightPage(PioneerQwen38Gguf& source,unsigned fragment,unsigned page,uint8_t* output,unsigned capacity) const {
		if (!ready || &source!=boundSource || source.Epoch()!=epoch || !source.Stable()
			|| fragment>=parts.size() || !output || capacity<4096) return false;
		const auto& part=parts[fragment];
		if (part.kind!=PIONEER_Q38_WEIGHT || page>=part.allocated/4096) return false;
		if (tiled && source.Tensors()[part.tensor].type!=0)
			return tiledReader.ReadPage(source,part.tensor,part.firstRow,part.rows,page,output,capacity);
		uint64_t relative=uint64_t(page)*4096;
		if (relative>=part.bytes) return false;
		unsigned bytes=part.bytes-relative<4096?unsigned(part.bytes-relative):4096;
		memset(output,0,4096);
		return source.ReadPacked(part.tensor,uint64_t(part.firstRow)*part.rowBytes+relative,bytes,output,capacity);
	}
};
#endif
