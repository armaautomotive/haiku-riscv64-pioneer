/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Native BFP band data is isolated from driver, Work and reference roles
#endif
#ifndef PIONEER_Q38_NATIVE_BFP_BAND_SOURCE_H
#define PIONEER_Q38_NATIVE_BFP_BAND_SOURCE_H
#include "PioneerQ38NativeBfpBandData.h"
#include "PioneerQ38NativeBfp8WeightTiles.h"
// Borrows an ALREADY-OPEN cache and independently authenticated model source.
// Bind's SHA/Ready are caller declarations,not independent authentication or
// GPU authority. The real diagnostic authenticates its SAME open full file.
// This source never opens a directory,creates/converts/repairs/publishes entries.
class PioneerQ38NativeBfpBandSource {
	using Cache=PioneerModelWeightCache;
	Cache& cache;PioneerQwen38Gguf* model=nullptr;Cache::Ready sourceReady;
	Cache::Digest original{};uint64_t epoch=0;bool attempted=false;
	std::unique_ptr<Cache::Reader> active;unsigned activeTensor=UINT32_MAX;
	static Cache::Digest Digest(const char* text) {
		Cache::Digest out{};
		for(unsigned i=0;i<32;i++) {
			auto nibble=[](char c){return unsigned(c>='0' && c<='9'?c-'0':c-'a'+10);};
			out[i]=uint8_t((nibble(text[i*2])<<4)|nibble(text[i*2+1]));
		}
		return out;
	}
	bool OpenTensor(unsigned tensor,const Cache::Key& key,std::string& error){
		if(activeTensor!=tensor) {
			active.reset();activeTensor=UINT32_MAX;auto next=std::make_unique<Cache::Reader>();
			if(!cache.OpenExisting(key,[this]{return Ready();},*next,error) || !Ready() || !next->IsOpen())return false;
			active=std::move(next);activeTensor=tensor;
		}
		if(!active || !active->IsOpen()){error="Prepared native cache reader changed;not reopened.";return false;}
		return true;
	}
public:
	explicit PioneerQ38NativeBfpBandSource(Cache& opened):cache(opened){}
	PioneerQ38NativeBfpBandSource(const PioneerQ38NativeBfpBandSource&)=delete;
	PioneerQ38NativeBfpBandSource& operator=(const PioneerQ38NativeBfpBandSource&)=delete;
	static Cache::Digest OriginalPin(){return Digest("c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");}
	// FROZEN,independently qualified all-BFP8 v2/source7db613bf recipe. This
	// cannot be overridden to accept mixed-BFP4/lossless/native-raw entries.
	static Cache::Digest ConversionPin(){return Digest("551c6713a31c749d0dad82e616c592f01aba6854af8781f6756b4f60d85bc897");}
	bool Bind(PioneerQwen38Gguf& source,const Cache::Digest& authenticatedSHA,const Cache::Ready& ready,std::string& error) {
		if(attempted){error="Native band source binding is one-shot.";return false;}attempted=true;
		if(authenticatedSHA!=OriginalPin() || !ready || !ready() || !source.Ready() || !source.Stable() || source.Tensors().size()!=851) {
			error="Native band source requires the independently authenticated complete pinned original.";return false;
		}
		model=&source;epoch=source.Epoch();sourceReady=ready;original=authenticatedSHA;
		if(!Ready()){model=nullptr;sourceReady={};error="Native source changed during one-shot binding.";return false;}
		error.clear();return true;
	}
	bool Ready() const{return model && sourceReady && sourceReady() && model->Ready() && model->Epoch()==epoch && model->Stable();}
	bool MatchesSource(const PioneerQwen38Gguf& source) const{return Ready() && model==&source;}
	bool MatchesIdentity(const PioneerQwen38Gguf& source,const Cache::Digest& sha) const{
		return MatchesSource(source) && original==sha;
	}
	// Key metadata only,not a claim that cached data exists or is verified.
	bool KeyFor(unsigned tensor,Cache::Key& key) const {
		if(!Ready() || tensor>=model->Tensors().size())return false;
		const auto& t=model->Tensors()[tensor];PioneerQ38NativeBfp8WeightTiles::Layout layout;
		if(t.name=="token_embd.weight" || !PioneerQ38NativeBfpBand::Width(unsigned(t.inputs)) || !PioneerQ38NativeBfp8WeightTiles::Describe(t,layout))return false;
		Cache::Key next;next.sourceSHA256=original;next.conversionSHA256=ConversionPin();next.entry=tensor;next.bytes=layout.bytes;key=next;return true;
	}
	// Preparation-only cache DATA, not a device uploader or ownership grant.
	// Uses the already qualified all-BFP8 cache recipe; never converts,
	// creates or repairs an entry. Last-page padding is generated as zeros.
	bool ReadStripePage(unsigned tensor,unsigned first,unsigned rows,unsigned page,uint8_t* output,unsigned capacity,std::string& error){
		Cache::Key key;if(!KeyFor(tensor,key)){error="Native BFP8 stripe tensor/source unavailable.";return false;}
		const auto t=model->Tensors()[tensor];
		if(!rows || (first&31) || (rows&31) || first>t.outputs || rows>t.outputs-first || !output || capacity<4096
			|| reinterpret_cast<uintptr_t>(output)>UINTPTR_MAX-4096){
			error="Invalid native BFP8 stripe geometry or output.";return false;
		}
		const uint64_t bytes=uint64_t(rows/32)*(t.inputs/32)*1088;
		if(uint64_t(page)*4096>=((bytes+4095)&~uint64_t(4095))){error="Native BFP8 stripe page out of range.";return false;}
		if(!OpenTensor(tensor,key,error))return false;
		return PioneerQ38NativeBfpBandData::ReadStripePage(*active,unsigned(t.inputs),unsigned(t.outputs),first,rows,page,
			[this]{return Ready();},output,capacity,error);
	}
	bool Stage(unsigned tensor,unsigned first,const uint16_t* input,unsigned count,unsigned phases,
		PioneerQ38NativeBfpBandData::Packet& output,std::string& error) {
		Cache::Key key;if(!KeyFor(tensor,key)){error="Native band tensor/source is unavailable or unsupported.";return false;}
		const auto t=model->Tensors()[tensor];
		std::vector<uint16_t> captured;
		if(!PioneerQ38NativeBfpBandData::Capture(unsigned(t.inputs),unsigned(t.outputs),first,input,count,phases,captured,error))return false;
		// Preserve one verified reader across bands/stripes; never implicitly
		// reopen or repair a changed FD for the same tensor.
		if(!OpenTensor(tensor,key,error))return false;
		PioneerQ38NativeBfpBandData::Packet next;
		if(!PioneerQ38NativeBfpBandData::Assemble(*active,unsigned(t.inputs),unsigned(t.outputs),first,captured.data(),count,phases,
			[this]{return Ready();},next,error))return false;
		if(!Ready() || !active->IsOpen()){error="Source/cache changed before bound native band publication.";return false;}
		next.key=key;next.sourceEpoch=epoch;output=std::move(next);error.clear();return true;
	}
};
#endif
