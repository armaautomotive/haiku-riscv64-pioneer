/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_REDISTRIBUTED_GDN_PACKAGE_H
#define PIONEER_Q38_GLOBAL_REDISTRIBUTED_GDN_PACKAGE_H
#include "PioneerQ38GlobalChainPackage.h"
#include "PioneerQ38RedistributedGdnPlan.h"

// Separate serializable candidate package: the unchanged global layer
// package followed by a 119-slot physical-worker GDN table. It does not
// change QGC1 bytes or confer native upload, launch, state, or Work authority.
class PioneerQ38GlobalRedistributedGdnPackage {
	PioneerQ38GlobalChainPackage base;
	PioneerQ38RedistributedGdnPlan gdn;
	const PioneerQ38CanonicalModelPlan* canonical=nullptr;
	unsigned layer=64,bank=7,phase=0,request=0;
	PioneerQ38GdnIdentity identity={};
	uint32_t sources[1920]={};
	bool attempted=false,ready=false;
	template<bool Strict> bool ReadBound(unsigned offset,uint8_t* output,unsigned bytes)const{
		const uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
		if(!canonical || !ready || !canonical->Ready() || layer>=64 || layer%4==3
			|| !output || o>UINTPTR_MAX-bytes || (o<self+sizeof(*this) && self<o+bytes)
			|| (offset&63) || !bytes || (bytes&63) || bytes>16384
			|| offset>Bytes() || bytes>Bytes()-offset)return false;
		if(Strict && !Matches(*canonical))return false;
		const unsigned baseBytes=base.Bytes();
		unsigned done=0;
		if(offset<baseBytes){
			const unsigned count=bytes<baseBytes-offset?bytes:baseBytes-offset;
			if(!base.Read(offset,output,count))return false;
			done=count;
		}
		while(done<bytes){
			const unsigned at=offset+done-baseBytes;
			const unsigned word=at/4,byte=at&3;
			uint32_t value;
			if(word<32)value=gdn.Header()[word];
			else{
				const unsigned recordWord=word-32;
				const unsigned worker=recordWord/192+1,index=recordWord%192;
				const uint32_t* record=gdn.Worker(worker);if(!record)return false;
				value=record[index];
			}
			for(unsigned b=byte;b<4 && done<bytes;b++)output[done++]=uint8_t(value>>(8*b));
		}
		return true;
	}
public:
	PioneerQ38GlobalRedistributedGdnPackage()=default;
	PioneerQ38GlobalRedistributedGdnPackage(const PioneerQ38GlobalRedistributedGdnPackage&)=delete;
	PioneerQ38GlobalRedistributedGdnPackage& operator=(const PioneerQ38GlobalRedistributedGdnPackage&)=delete;
	bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selectedLayer,unsigned selectedBank,
		const PioneerQ38GdnIdentity& id,const uint32_t* sources,unsigned initialBank,
		uint32_t initialAddress,const uint32_t* statePageMap=nullptr){
		if(attempted)return false;attempted=true;
		if(selectedLayer>=64 || selectedLayer%4==3 || !sources
			|| !base.Build(c,selectedLayer,selectedBank,id,sources,initialBank,initialAddress))return false;
		canonical=&c;layer=selectedLayer;bank=selectedBank;identity=id;
		phase=1001+selectedLayer*16;request=selectedLayer+1;
		for(unsigned i=0;i<1920;i++)this->sources[i]=sources[i];
		if(!gdn.Build(c,selectedLayer,0,selectedBank,0,phase,request,id,sources,statePageMap))return false;
		ready=true;return Matches(c);
	}
	bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
		return ready && canonical==&c && c.Ready() && layer<64 && layer%4!=3 && bank<7
			&& phase==1001+layer*16 && request==layer+1
			&& base.Matches(c) && gdn.Matches(c,layer,0,bank,0,phase,request,identity,sources);
	}
	unsigned Bytes()const{
		return ready?base.Bytes()+(32+PioneerQ38GdnTaskPlacement::PhysicalWorkers*192)*4:0;
	}
	const uint32_t* Boot()const{return ready?base.Boot():nullptr;}
	const uint32_t* GdnHeader()const{return ready?gdn.Header():nullptr;}
	const uint32_t* GdnWorker(unsigned physicalWorker)const{return ready?gdn.Worker(physicalWorker):nullptr;}
	bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{return ReadBound<true>(offset,output,bytes);}
};
static_assert(sizeof(PioneerQ38GlobalRedistributedGdnPackage)>1100000
	&& sizeof(PioneerQ38GlobalRedistributedGdnPackage)<3200000,
	"separate canonical layer plus redistributed GDN upload package remains heap-only");
#endif
