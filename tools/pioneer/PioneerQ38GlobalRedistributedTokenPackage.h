/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_REDISTRIBUTED_TOKEN_PACKAGE_H
#define PIONEER_Q38_GLOBAL_REDISTRIBUTED_TOKEN_PACKAGE_H
#include "PioneerQ38TokenPackage.h"
#include "PioneerQ38RedistributedGdnPlan.h"
#include "PioneerQ38RedistributedTokenLayout.h"

// Opt-in full-stride token package with physical-worker GDN descriptors in
// pre-existing zeroed slack. Default TokenPackage bytes remain unchanged.
// This serializes DATA only; it does not initialize recurrent memory or grant
// native upload, worker release, token commit, CPU-reference, or Work authority.
class PioneerQ38GlobalRedistributedTokenPackage {
	PioneerQ38TokenPackage base;
	PioneerQ38RedistributedGdnPlan gdn;
	const PioneerQ38CanonicalModelPlan* canonical=nullptr;
	unsigned layer=64,bank=7,slot=2,phase=0,request=0;
	PioneerQ38GdnIdentity identity={};
	uint32_t sources[1920]={};
	bool attempted=false,ready=false;
	template<bool Strict> bool ReadBound(unsigned offset,uint8_t* output,unsigned bytes)const{
		namespace L=PioneerQ38RedistributedTokenLayout;
		const uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
		const uintptr_t c=reinterpret_cast<uintptr_t>(canonical);
		if(!canonical || !ready || !canonical->Ready() || layer>=64 || layer%4==3
			|| !output || !bytes || ((offset|bytes)&63) || bytes>16384
			|| offset>PioneerQ38GlobalChainLayout::Stride
			|| bytes>PioneerQ38GlobalChainLayout::Stride-offset || o>UINTPTR_MAX-bytes
			|| (o<self+sizeof(*this) && self<o+bytes)
			|| (o<c+sizeof(*canonical) && c<o+bytes)
			|| (Strict && !Matches(*canonical)))return false;
		if(!base.Read(offset,output,bytes))return false;
		const unsigned begin=offset>unsigned(L::Offset)?offset:unsigned(L::Offset);
		const unsigned finish=offset+bytes<unsigned(L::End)?offset+bytes:unsigned(L::End);
		for(unsigned at=begin;at<finish;at++){
			const unsigned relative=at-L::Offset,word=relative/4,byte=relative&3;
			uint32_t value;
			if(word<L::HeaderWords)value=gdn.Header()[word];
			else{
				const unsigned recordWord=word-L::HeaderWords;
				const unsigned worker=recordWord/L::RecordWords+1,index=recordWord%L::RecordWords;
				const uint32_t* record=gdn.Worker(worker);if(!record)return false;
				value=record[index];
			}
			output[at-offset]=uint8_t(value>>(byte*8));
		}
		return true;
	}
public:
	PioneerQ38GlobalRedistributedTokenPackage()=default;
	PioneerQ38GlobalRedistributedTokenPackage(const PioneerQ38GlobalRedistributedTokenPackage&)=delete;
	PioneerQ38GlobalRedistributedTokenPackage& operator=(const PioneerQ38GlobalRedistributedTokenPackage&)=delete;
	bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,unsigned selectedBank,
		unsigned stateSlot,const PioneerQ38GdnIdentity& id,const uint32_t* source,
		unsigned initialBank,unsigned initialAddress){
		if(attempted)return false;attempted=true;
		if(selected>=64 || selected%4==3 || selectedBank>=7 || stateSlot!=0 || !source
			|| !base.Build(c,selected,selectedBank,id,source,initialBank,initialAddress))return false;
		canonical=&c;layer=selected;bank=selectedBank;slot=stateSlot;identity=id;
		// TokenMetadata::Gdn consumes the same initial GDN phase as the
		// canonical recurrent package: outer phase + 5. Record() adds one
		// while building each input descriptor, so the table plan phase is +4.
		phase=1005+selected*16;request=selected+1;
		for(unsigned i=0;i<1920;i++)sources[i]=source[i];
		if(!gdn.Build(c,selected,0,selectedBank,stateSlot,phase,request,id,source))return false;
		ready=true;return Matches(c);
	}
	bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
		return ready && canonical==&c && c.Ready() && layer<64 && layer%4!=3
			&& bank<7 && slot<2 && phase==1005+layer*16 && request==layer+1
			&& base.Matches(c) && gdn.Matches(c,layer,0,bank,slot,phase,request,identity,sources);
	}
	unsigned Bytes()const{return ready?PioneerQ38GlobalChainLayout::Stride:0;}
	const uint32_t* Boot()const{return ready?base.Boot():nullptr;}
	const uint32_t* GdnHeader()const{return ready?gdn.Header():nullptr;}
	const uint32_t* GdnWorker(unsigned physicalWorker)const{return ready?gdn.Worker(physicalWorker):nullptr;}
	bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{return ReadBound<true>(offset,output,bytes);}
};
static_assert(sizeof(PioneerQ38GlobalRedistributedTokenPackage)>1600000
	&& sizeof(PioneerQ38GlobalRedistributedTokenPackage)<2000000,
	"token package plus canonical GDN schedule is heap-resident DATA only");
#endif
