/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FLASH_NEXT_SOURCE_H
#define PIONEER_FLASH_NEXT_SOURCE_H
#include "PioneerFlashNextOffload.h"
#include <string>

namespace PioneerFlashNext {
// The model loader implements this over retained, whole-file SHA-verified
// mappings. Replacing a path must not rebind an existing source. It also owns
// any platform-specific file identity / write exclusion checks.
class WeightOwner {
public:
	virtual ~WeightOwner()=default;
	virtual bool Authenticated() const=0;
	virtual uint64_t Epoch() const=0;
	virtual bool Contains(const uint8_t* data,uint64_t bytes) const=0;
};
struct Tensor {
	std::string name;
	const uint8_t* data=nullptr;
	uint64_t bytes=0;
	unsigned type=0,rank=0;
	uint64_t shape[4]={};
	unsigned shard=0;
};

// Zero-copy adapter from the split-GGUF loader's tensor directory to packed
// per-expert matrix slices. It validates ONLY the routed expert directory;
// it cannot validate the full decoder graph or authenticate a file itself.
class PackedSource : public Source {
	struct Binding { const WeightOwner* owner=nullptr; Tensor tensor; };
	std::vector<Binding> bindings;
	std::vector<std::pair<const WeightOwner*,uint64_t>> sourceOwners;
	bool attempted=false,ready=false;
	uint64_t epoch=0;
	static std::string Name(unsigned layer,unsigned role) {
		static const char* names[]={"gate","up","down"};
		return "blk."+std::to_string(layer)+".ffn_"+names[role]+"_exps.weight";
	}
public:
	PackedSource()=default;
	PackedSource(const PackedSource&)=delete;
	PackedSource& operator=(const PackedSource&)=delete;
	// Owners and their mappings outlive this object. Directory entries are
	// copied; there is no dependence on caller vector/string storage afterward.
	bool Bind(const char* architecture,const std::vector<const WeightOwner*>& owners,
		const std::vector<Tensor>& tensors,uint64_t modelEpoch) {
		if (attempted) return false;
		attempted=true;
		if (!architecture || strcmp(architecture,"qwen4exp") || owners.empty() || owners.size()>64 || !modelEpoch) return false;
		for (auto* owner:owners) {
			if (!owner || !owner->Authenticated() || !owner->Epoch()) return false;
			bool seen=false; for (const auto& checked:sourceOwners) if (checked.first==owner) seen=true;
			if (!seen) sourceOwners.emplace_back(owner,owner->Epoch());
		}
		std::vector<Binding> stage(Layers*3);
		for (unsigned layer=0;layer<Layers;layer++) for (unsigned role=0;role<3;role++) {
			const Tensor* found=nullptr;
			std::string name=Name(layer,role);
			for (const auto& t:tensors) if (t.name==name) { if (found) return false; found=&t; }
			if (!found || found->shard>=owners.size() || found->rank!=3 || found->shape[2]!=Experts
				|| found->shape[3] || found->shape[0]!=(role==2?Intermediate:Hidden)
				|| found->shape[1]!=(role==2?Hidden:Intermediate) || found->bytes%Experts) return false;
			Matrix matrix{found->data,found->bytes/Experts,unsigned(found->shape[0]),unsigned(found->shape[1]),found->type};
			auto* owner=owners[found->shard];
			if (!matrix.Valid() || found->bytes>SIZE_MAX
				|| reinterpret_cast<uintptr_t>(found->data)>UINTPTR_MAX-found->bytes
				|| !owner->Contains(found->data,found->bytes)) return false;
			stage[layer*3+role]={owner,*found};
		}
		// Expert tensors must not alias one another in a mapping. Pointer
		// comparisons use integer differences, avoiding unrelated-pointer UB.
		for (unsigned i=0;i<stage.size();i++) for (unsigned j=0;j<i;j++) {
			uintptr_t a=reinterpret_cast<uintptr_t>(stage[i].tensor.data),b=reinterpret_cast<uintptr_t>(stage[j].tensor.data);
			if (a<=b ? stage[i].tensor.bytes>b-a : stage[j].tensor.bytes>a-b) return false;
		}
		bindings.swap(stage); epoch=modelEpoch; ready=true;
		if (!Ready()) { ready=false; return false; }
		return true;
	}
	bool Ready() const override {
		if (!ready || bindings.size()!=Layers*3) return false;
		for (const auto& owner:sourceOwners)
			if (!owner.first->Authenticated() || owner.first->Epoch()!=owner.second) return false;
		return true;
	}
	uint64_t Epoch() const override { return epoch; }
	bool Get(Key key,Expert& expert) const override {
		if (!key.Valid() || !Ready()) return false;
		Expert stage; Matrix* matrices[]={&stage.gate,&stage.up,&stage.down};
		for (unsigned role=0;role<3;role++) {
			const auto& b=bindings[key.layer*3+role]; const auto& t=b.tensor;
			uint64_t bytes=t.bytes/Experts,offset=bytes*key.expert;
			const uint8_t* slice=reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(t.data)+offset);
			if (offset>SIZE_MAX || bytes>SIZE_MAX || !b.owner->Contains(slice,bytes)) return false;
			*matrices[role]={slice,bytes,unsigned(t.shape[0]),unsigned(t.shape[1]),t.type};
		}
		if (!stage.Valid() || !Ready()) return false;
		expert=stage; return true;
	}
};

// The graph supplies its 16 already-hashed PLE row IDs. Keep the large
// table mapped on the host and expand only 16 x 160 selected values.
// This primitive does not implement the PLE hash, convolution or projection.
class NgramTable {
	const WeightOwner* owner=nullptr;
	uint64_t epoch=0,rowBytes=0;
	Tensor tensor;
	std::array<uint64_t,16> offsets={},vocabularies={};
public:
	bool Bind(const WeightOwner& weights,const Tensor& t,const std::array<uint64_t,16>& headOffsets,
		const std::array<uint64_t,16>& headVocabularies) {
		if (owner || !weights.Authenticated() || !weights.Epoch() || t.name!="per_layer_token_embd.weight"
			|| t.rank!=2 || t.shape[0]!=160 || !t.shape[1] || t.shape[1]>UINT32_MAX || t.shape[2] || t.shape[3]
			|| !t.data || !BlockElements(t.type) || 160%BlockElements(t.type)) return false;
		uint64_t stride=uint64_t(160/BlockElements(t.type))*BlockBytes(t.type);
		if (t.bytes!=stride*t.shape[1] || t.bytes>SIZE_MAX
			|| reinterpret_cast<uintptr_t>(t.data)>UINTPTR_MAX-t.bytes || !weights.Contains(t.data,t.bytes)) return false;
		for (unsigned i=0;i<16;i++) {
			if (!headVocabularies[i] || headOffsets[i]>t.shape[1] || headVocabularies[i]>t.shape[1]-headOffsets[i]) return false;
			for (unsigned j=0;j<i;j++) {
				if (headOffsets[i]<headOffsets[j]+headVocabularies[j] && headOffsets[j]<headOffsets[i]+headVocabularies[i]) return false;
			}
		}
		tensor=t; offsets=headOffsets; vocabularies=headVocabularies;
		epoch=weights.Epoch(); rowBytes=stride; owner=&weights; return Ready();
	}
	bool Ready() const { return owner && owner->Authenticated() && owner->Epoch()==epoch; }
	uint64_t PackedBytesPerGather() const { return Ready()?rowBytes*16:0; }
	bool Gather(const uint32_t* rows,unsigned count,float* output,unsigned capacity) const {
		if (!Ready() || !rows || count!=16 || !output || capacity<2560) return false;
		for (unsigned i=0;i<16;i++) if (rows[i]<offsets[i] || rows[i]-offsets[i]>=vocabularies[i]) return false;
		std::vector<float> stage(2560);
		unsigned n=BlockElements(tensor.type),bytes=BlockBytes(tensor.type);
		for (unsigned i=0;i<16;i++) {
			uint64_t offset=uint64_t(rows[i])*rowBytes;
			const uint8_t* p=tensor.data+offset;
			if (!owner->Contains(p,rowBytes)) return false;
			for (unsigned col=0;col<160;col+=n)
				if (!DecodeBlock(tensor.type,p+(col/n)*bytes,stage.data()+i*160+col)) return false;
		}
		if (!Ready()) return false;
		memcpy(output,stage.data(),stage.size()*sizeof(float)); return true;
	}
};
} // namespace PioneerFlashNext
#endif
