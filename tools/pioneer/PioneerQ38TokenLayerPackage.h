/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_LAYER_PACKAGE_H
#define PIONEER_Q38_TOKEN_LAYER_PACKAGE_H
#include "PioneerQ38TokenPackage.h"
#include "PioneerQ38GlobalRedistributedTokenPackage.h"
#include <new>

// Per-layer immutable DATA adapter. Routed GDN is an explicit opt-in and
// applies only to recurrent layers; dense layers and the default path retain
// the legacy package byte-for-byte. This does not grant launch/Work authority.
class PioneerQ38TokenLayerPackage {
	union Storage {
		PioneerQ38TokenPackage legacy;
		PioneerQ38GlobalRedistributedTokenPackage routed;
		Storage(){}
		~Storage(){}
	} storage;
	bool useRouted=false,constructed=false,attempted=false,ready=false;
public:
	PioneerQ38TokenLayerPackage()=default;
	PioneerQ38TokenLayerPackage(const PioneerQ38TokenLayerPackage&)=delete;
	PioneerQ38TokenLayerPackage& operator=(const PioneerQ38TokenLayerPackage&)=delete;
	~PioneerQ38TokenLayerPackage(){
		if(!constructed)return;
		if(useRouted)storage.routed.~PioneerQ38GlobalRedistributedTokenPackage();
		else storage.legacy.~PioneerQ38TokenPackage();
	}
	bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned bank,
		const PioneerQ38GdnIdentity& id,const uint32_t* sources,unsigned initialBank,
		uint32_t initialAddress,bool redistributedGdn=false){
		if(attempted)return false;
		attempted=true;
		if(redistributedGdn && layer<64 && layer%4!=3){
			useRouted=true;
			new (&storage.routed) PioneerQ38GlobalRedistributedTokenPackage;
			constructed=true;
			ready=storage.routed.Build(c,layer,bank,0,id,sources,initialBank,initialAddress);
			return ready;
		}
		new (&storage.legacy) PioneerQ38TokenPackage;
		constructed=true;
		ready=storage.legacy.Build(c,layer,bank,id,sources,initialBank,initialAddress);
		return ready;
	}
	bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
		return ready && (useRouted?storage.routed.Matches(c):storage.legacy.Matches(c));
	}
	const uint32_t* Boot()const{return ready?(useRouted?storage.routed.Boot():storage.legacy.Boot()):nullptr;}
	const uint32_t* Outer()const{return ready?(useRouted?storage.routed.Outer():storage.legacy.Outer()):nullptr;}
	unsigned Bytes()const{return ready?(useRouted?storage.routed.Bytes():storage.legacy.Bytes()):0;}
	bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{
		return ready && (useRouted?storage.routed.Read(offset,output,bytes):storage.legacy.Read(offset,output,bytes));
	}
	template<bool Strict> bool ReadBound(unsigned offset,uint8_t* output,unsigned bytes)const{
		return ready && (useRouted?storage.routed.template ReadPackageBound<Strict>(offset,output,bytes)
			:storage.legacy.template ReadPackageBound<Strict>(offset,output,bytes));
	}
};
#endif
