/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CACHED_PUBLIC_MODEL_SOURCE_H
#define PIONEER_Q38_CACHED_PUBLIC_MODEL_SOURCE_H
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQ38LosslessWeightCache.h"
// Explicit opt-in source for the existing Work loader, not a new device
// format. Authentication remains the base class's SAME open full GGUF hash.
// Preparation MUST finish before session.Open; page reads cannot build/repair.
class PioneerQ38CachedPublicModelSource:public PioneerQ38PublicModelSource {
	PioneerQ38LosslessWeightCache weights;
	bool attempted=false,prepared=false;
public:
	bool Ready() const { return prepared && PioneerQ38PublicModelSource::Ready() && weights.AllPrepared(); }
	bool PrepareWeightCache(const char* directory,const char* fingerprint,std::string& error,
		const std::function<void(unsigned,unsigned,uint64_t,uint64_t,bool)>& progress={},
		const PioneerModelWeightCache::Ready& keepGoing={}) {
		if(attempted || !PioneerQ38PublicModelSource::Ready() || !fingerprint || strlen(fingerprint)!=64 || (keepGoing && !keepGoing()))return false;
		attempted=true;
		for(unsigned i=0;i<64;i++)if(!strchr("0123456789abcdef",fingerprint[i]))return false;
		// No caller-supplied original digest; this exact digest was already
		// required by the base source's full same-FD authentication.
		PioneerModelWeightCache::Digest original{};
		const char* digest="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747";
		const char* hex="0123456789abcdef";
		for(unsigned i=0;i<32;i++)original[i]=uint8_t((strchr(hex,digest[i*2])-hex)*16+(strchr(hex,digest[i*2+1])-hex));
		const auto recipe=PioneerQ38LosslessWeightCache::Recipe(fingerprint);
		const auto conversion=PioneerModelWeightCache::Hash(recipe.data(),recipe.size());
		// Model() exposes immutable metadata; base owns the mutable reader.
		// Its ReadWeightPage already mutates that same source FILE. This
		// serialized adapter never changes tensor metadata or reopens a path.
		auto& model=const_cast<PioneerQwen38Gguf&>(Model());
		if(!weights.Bind(directory,model,original,conversion,[this,keepGoing]{
			return PioneerQ38PublicModelSource::Ready() && (!keepGoing || keepGoing());
		},error))return false;
		PioneerQ38LosslessWeightCache::Statistics stats;
		for(unsigned i=0;i<model.Tensors().size();i++) {
			auto observe=[&](uint64_t done,uint64_t total,bool verifying) {
				if(progress)progress(i,unsigned(model.Tensors().size()),done,total,verifying);
			};
			if(!weights.PrepareTensor(i,stats,error,observe))return false;
		}
		prepared=weights.AllPrepared();return Ready();
	}
	bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned fragment,unsigned page,uint8_t* output,unsigned capacity) {
		if(!output || capacity<4096 || !Ready())return false;
		uint8_t stage[4096];std::string error;
		if(!weights.ReadWeightPage(plan,fragment,page,stage,sizeof(stage),error) || !Ready())return false;
		memcpy(output,stage,sizeof(stage));return true;
	}
	bool ReadWeightPages(const PioneerQwen38PackedPlan& plan,unsigned fragment,unsigned page,unsigned pages,
		uint8_t* output,unsigned capacity) {
		if(!output || !pages || pages>16 || capacity<pages*4096 || !Ready())return false;
		std::vector<uint8_t> stage(pages*4096);std::string error;
		if(!weights.ReadWeightPages(plan,fragment,page,pages,stage.data(),stage.size(),error) || !Ready())return false;
		memcpy(output,stage.data(),stage.size());return true;
	}
};
#endif
