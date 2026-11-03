/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LOSSLESS_WEIGHT_CACHE_H
#define PIONEER_Q38_LOSSLESS_WEIGHT_CACHE_H
#include "PioneerModelWeightCache.h"
#include "PioneerQwen38PackedPlan.h"

// Userspace derived WEIGHT bytes only. The existing byte-major permutation
// is LOSSLESS: no dequantization, native BFP requantization, learned products,
// model-quality change or device capability. One serialized owner per FILE.
// Caller authenticates its SAME open original before Bind; Ready must bind
// that authenticated identity for this object's ENTIRE lifetime.
class PioneerQ38LosslessWeightCache {
	using Cache=PioneerModelWeightCache;
	struct Entry { Cache::Key key;unsigned rowBytes=0;bool prepared=false; };
	Cache cache;
	PioneerQwen38Gguf* model=nullptr;
	uint64_t epoch=0;
	Cache::Ready sourceReady;
	PioneerQwen38TiledReader converter;
	std::vector<Entry> entries;
	std::unique_ptr<Cache::Reader> reader;
	unsigned active=UINT32_MAX,windowBytes=0,preparedEntries=0;
	uint64_t windowOffset=0;
	std::array<uint8_t,Cache::ChunkBytes> window{};
	bool attempted=false;
	bool Ready() const {
		return model && sourceReady && sourceReady() && model->Epoch()==epoch && model->Stable();
	}
	static bool Fail(std::string& error,const char* message) { error=message;return false; }
public:
	struct Statistics { uint64_t entries=0,hits=0,bytes=0,convertedPages=0; };
	static std::string Recipe(const std::string& fingerprint) {
		return "pioneer-q38-lossless-byte-major-v1/original-Q8_0,Q4_K,Q6_K-unchanged/"
			"output-groups16/quant-block-bytes-major/tile-align64-zero/float32-raw/"
			"tensor-ordinal/no-addresses/no-state/source-fingerprint="+fingerprint;
	}
	static bool Describe(const PioneerQ38Tensor& tensor,unsigned& rowBytes,uint64_t& bytes) {
		if(!tensor.inputs || !tensor.outputs || tensor.outputs>UINT32_MAX)return false;
		if(!tensor.type) {
			if(tensor.inputs>UINT32_MAX/4)return false;
			rowBytes=unsigned(tensor.inputs)*4;
		} else {
			if(tensor.outputs%16)return false;
			rowBytes=PioneerQ38TiledRowBytes(tensor.type,tensor.inputs);
		}
		if(!rowBytes || tensor.outputs>Cache::MaxEntryBytes/rowBytes)return false;
		bytes=tensor.outputs*rowBytes;return bytes!=0;
	}
	bool Bind(const char* directory,PioneerQwen38Gguf& source,const Cache::Digest& originalSHA,
		const Cache::Digest& conversionSHA,const Cache::Ready& ready,std::string& error) {
		if(attempted)return Fail(error,"Lossless cache binding is one-shot.");
		attempted=true;
		if(!ready || !ready() || !source.Stable() || !converter.Bind(source))
			return Fail(error,"Lossless cache needs an independently authenticated stable original.");
		std::vector<Entry> next(source.Tensors().size());
		if(next.empty())return Fail(error,"No source tensor layout.");
		for(unsigned i=0;i<next.size();i++) {
			auto& entry=next[i];
			if(!Describe(source.Tensors()[i],entry.rowBytes,entry.key.bytes))
				return Fail(error,"Unsupported lossless cache tensor layout.");
			entry.key.sourceSHA256=originalSHA;entry.key.conversionSHA256=conversionSHA;entry.key.entry=i;
		}
		if(!cache.Open(directory,error) || !ready() || !source.Stable())return false;
		model=&source;epoch=source.Epoch();sourceReady=ready;entries=std::move(next);return Ready();
	}
	// Run BEFORE opening an accelerator session. Each completed tensor is an
	// atomic entry; cancellation/failure preserves previous complete entries.
	bool PrepareTensor(unsigned tensor,Statistics& stats,std::string& error,
		const Cache::Progress& progress=Cache::Progress()) {
		if(!Ready() || tensor>=entries.size())return Fail(error,"Lossless cache source/tensor unavailable.");
		auto& entry=entries[tensor];if(entry.prepared)return Fail(error,"Tensor already prepared by this owner.");
		uint64_t pages=0;auto produce=[&](uint64_t offset,unsigned count,uint8_t* output,std::string& why) {
			if(!Ready() || offset%4096 || offset>=entry.key.bytes || !count || count>Cache::ChunkBytes || count>entry.key.bytes-offset)
				return Fail(why,"Invalid lossless conversion range/source.");
			const auto& t=model->Tensors()[tensor];
			for(unsigned copied=0;copied<count;) {
				const unsigned take=std::min(4096U,count-copied);uint8_t page[4096]={};
				if(t.type) {
					if(!converter.ReadPage(*model,tensor,0,unsigned(t.outputs),unsigned((offset+copied)/4096),page,sizeof(page)))return false;
				} else if(!model->ReadPacked(tensor,offset+copied,take,page,sizeof(page)))return false;
				if(!Ready())return false;
				memcpy(output+copied,page,take);copied+=take;pages++;
			}
			return Ready();
		};
		Cache::Reader prepared;bool hit=false;
		if(!cache.GetOrCreate(entry.key,[this]{return Ready();},produce,prepared,hit,error,progress)
			|| !Ready() || !prepared.IsOpen() || prepared.Size()!=entry.key.bytes || (hit && pages))return false;
		entry.prepared=true;preparedEntries++;stats.entries++;stats.hits+=hit;stats.bytes+=entry.key.bytes;stats.convertedPages+=pages;return true;
	}
	bool AllPrepared() const {
		return Ready() && !entries.empty() && preparedEntries==entries.size();
	}
	// Immutable key metadata only; not source authentication or GPU authority.
	bool KeyFor(unsigned tensor,Cache::Key& output) const {
		if(!Ready() || tensor>=entries.size())return false;
		output=entries[tensor].key;return true;
	}
	// Upload phase: opens EXISTING verified entries only. A missing/corrupt
	// entry fails closed; NO repair/conversion after the accelerator is opened.
	// One reader/64KiB snapshot bounds FDs/RAM. Actual page bytes and every
	// fragment's tail are identical to PackedPlan::ReadWeightPage.
	bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned fragment,unsigned page,
		uint8_t* output,unsigned capacity,std::string& error) {
		if(!Ready() || !output || capacity<4096 || !plan.MatchesSource(*model) || !plan.Tiled())
			return Fail(error,"Lossless cache reader needs the bound tiled plan and stable original.");
		const auto& parts=plan.Parts();if(fragment>=parts.size())return Fail(error,"Invalid weight fragment.");
		const auto& part=parts[fragment];
		if(part.kind!=PIONEER_Q38_WEIGHT || part.tensor>=entries.size() || !part.allocated || part.allocated%4096
			|| page>=part.allocated/4096)return Fail(error,"Invalid cached weight page.");
		const auto& entry=entries[part.tensor];const auto& tensor=model->Tensors()[part.tensor];
		const uint64_t relative=uint64_t(page)*4096;
		if(!entry.prepared || part.rowBytes!=entry.rowBytes || !part.rows || part.firstRow>tensor.outputs
			|| part.rows>tensor.outputs-part.firstRow || uint64_t(part.rows)*part.rowBytes!=part.bytes || relative>=part.bytes)
			return Fail(error,"Cached tensor was not prepared or does not match this fragment.");
		if(active!=part.tensor) {
			reader.reset();active=UINT32_MAX;windowBytes=0;
			auto next=std::make_unique<Cache::Reader>();
			if(!cache.OpenExisting(entry.key,[this]{return Ready();},*next,error))return false;
			reader=std::move(next);active=part.tensor;
		}
		uint8_t stage[4096]={};unsigned copied=0;
		const unsigned count=unsigned(std::min(UINT64_C(4096),uint64_t(part.bytes)-relative));
		const uint64_t start=uint64_t(part.firstRow)*part.rowBytes+relative;
		while(copied<count) {
			const uint64_t position=start+copied,first=position/Cache::ChunkBytes*Cache::ChunkBytes;
			if(!windowBytes || windowOffset!=first) {
				windowBytes=0;const unsigned bytes=unsigned(std::min(uint64_t(Cache::ChunkBytes),entry.key.bytes-first));
				if(!reader->Read(first,bytes,window.data(),window.size(),error))return false;
				windowOffset=first;windowBytes=bytes;
			}
			const unsigned within=unsigned(position-windowOffset),take=std::min(count-copied,windowBytes-within);
			memcpy(stage+copied,window.data()+within,take);copied+=take;
		}
		if(!Ready() || !reader->IsOpen())return Fail(error,"Original or cached file changed during weight-page read.");
		memcpy(output,stage,sizeof(stage));return true;
	}
};
#endif
