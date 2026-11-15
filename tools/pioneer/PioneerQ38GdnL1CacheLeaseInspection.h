/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION) \
	|| PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION != 1
#error Recurrent L1 cache lease is inspection-only
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
	|| defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
	|| defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
	|| defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) \
	|| defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) \
	|| defined(_KERNEL_MODE)
#error Inspection-only L1 cache lease cannot select Work,validation,or native execution
#endif
#ifndef PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION_H
#define PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION_H
#include <stdint.h>
#include "PioneerQ38GdnState.h"

// Metadata only: this lease owns no SRAM, verifies no hardware completion,
// and authorizes no transfer. A native caller must invoke CompleteHalf only
// after proving that the selected page set was written back successfully.
struct PioneerQ38GdnL1CacheSlice {
	PioneerQ38GdnRedistributedStateSlice state;
	uint32_t cacheIndex;
	unsigned cacheMode;
	bool resident;
	bool cacheHit;
};

class PioneerQ38GdnL1CacheLeaseInspection {
	public:
	enum CachePolicy { WriteThroughPolicy, L1AuthoritativePolicy };
	enum CacheMode { ColdFillMode=0, WarmWriteThroughMode=1, GddrSpillMode=2,
		WarmL1AuthoritativeMode=3 };
	private:
	enum Phase { Empty, Ready, Token, Layer, PendingModelPublication, Poisoned, Closed } phase=Empty;
	enum : unsigned { Workers=119, SlotsPerWorker=PioneerQ38GdnTaskPlacement::SlotsPerPhysical,
		ResidentSlotsPerWorker=25,
		CacheEntries=Workers*SlotsPerWorker, CacheWords=(CacheEntries+63)/64 };
	PioneerQ38GdnState state;
	PioneerQ38GdnIdentity identity={};
	CachePolicy policy=WriteThroughPolicy;
	uint64_t cacheValid[CacheWords]={};
	uint64_t cachePosition=0;
	uint64_t pendingPosition=0;
	unsigned cacheSlot=0;
	bool cacheBound=false;

	static unsigned CacheIndex(const PioneerQ38GdnTaskPlacement::Route& route)
	{
		return (route.physicalWorker-1)*SlotsPerWorker+route.stateIndex;
	}
	bool Valid(unsigned index) const
	{
		return index<CacheEntries && (cacheValid[index/64]&(UINT64_C(1)<<(index%64)));
	}
	void Invalidate(unsigned index)
	{
		if(index<CacheEntries)cacheValid[index/64]&=~(UINT64_C(1)<<(index%64));
	}
	void Invalidate()
	{
		for(unsigned i=0;i<CacheWords;i++)cacheValid[i]=0;
		cacheBound=false;
	}
public:
	PioneerQ38GdnL1CacheLeaseInspection()=default;
	explicit PioneerQ38GdnL1CacheLeaseInspection(CachePolicy selectedPolicy)
		:policy(selectedPolicy){}
	PioneerQ38GdnL1CacheLeaseInspection(const PioneerQ38GdnL1CacheLeaseInspection&)=delete;
	PioneerQ38GdnL1CacheLeaseInspection& operator=(const PioneerQ38GdnL1CacheLeaseInspection&)=delete;

	// Bind only after the external owner initialized both GDDR slots. The L1
	// cache starts invalid, so the first token must fill each resident entry.
	bool Bind(const PioneerQ38GdnIdentity& id)
	{
		if(phase!=Empty || !id.Valid() || !state.Bind(id))return false;
		identity=id;phase=Ready;return true;
	}
	bool BeginToken(const PioneerQ38GdnIdentity& id,uint64_t position)
	{
		if(phase!=Ready || !(id==identity) || !state.BeginToken(id,position))return false;
		phase=Token;return true;
	}
	bool BeginLayer(unsigned layer,PioneerQ38GdnTicket& ticket)
	{
		if(phase!=Token || !state.BeginLayer(layer,ticket))return false;
		phase=Layer;return true;
	}
	bool Slice(const PioneerQ38GdnTicket& ticket,unsigned logicalWorker,bool next,
		PioneerQ38GdnL1CacheSlice& output) const
	{
		if(phase!=Layer)return false;
		PioneerQ38GdnL1CacheSlice candidate={};
		if(!state.RedistributedSlice(ticket,logicalWorker,next,candidate.state))return false;
		const auto& route=candidate.state.route;
		if(!route.physicalWorker || route.physicalWorker>Workers || route.stateIndex>=SlotsPerWorker)return false;
		candidate.cacheIndex=CacheIndex(route);
		candidate.resident=route.stateIndex<ResidentSlotsPerWorker;
		candidate.cacheHit=candidate.resident && cacheBound
			&& cachePosition==ticket.position && cacheSlot==ticket.readSlot
			&& Valid(candidate.cacheIndex);
		candidate.cacheMode=candidate.resident
			?(candidate.cacheHit && policy==L1AuthoritativePolicy
				?WarmL1AuthoritativeMode:candidate.cacheHit?WarmWriteThroughMode:ColdFillMode)
			:GddrSpillMode;
		output=candidate;return true;
	}
	// stateUpdateProved is a receipt from the caller, NOT independently
	// checked here. It proves that a resident page was updated in L1 (regardless
	// of whether GDDR write-through was also requested). Spill entries must use
	// the ordinary GDDR state path. This metadata policy authorizes no I/O.
	bool CompleteHalf(const PioneerQ38GdnTicket& ticket,unsigned logicalWorker,bool stateUpdateProved)
	{
		PioneerQ38GdnL1CacheSlice slice={};
		if(!Slice(ticket,logicalWorker,true,slice) || slice.resident!=stateUpdateProved)return false;
		if(!state.CompletedHalf(ticket,logicalWorker))return false;
		if(slice.resident)Invalidate(slice.cacheIndex);
		return true;
	}
	bool CompleteLayer(const PioneerQ38GdnTicket& ticket)
	{
		if(phase!=Layer || !state.CompleteLayer(ticket))return false;
		phase=Token;return true;
	}
	// Commit the recurrent-state subtransaction after all 48 layers, but do
	// NOT publish cache validity yet: the enclosing model token may still fail
	// in a different stage. The caller must call PublishModelToken only after
	// the whole-model transaction has succeeded. Any intervening fault poisons
	// the session; mode 3 cannot roll its in-place L1 writes back.
	bool CommitToken(const PioneerQ38GdnIdentity& id,uint64_t position)
	{
		if(phase!=Token || !(id==identity) || !state.CommitToken(id,position))return false;
		pendingPosition=position;phase=PendingModelPublication;return true;
	}
	bool PublishModelToken(const PioneerQ38GdnIdentity& id,uint64_t position)
	{
		if(phase!=PendingModelPublication || !(id==identity) || position!=pendingPosition)return false;
		for(unsigned worker=0;worker<Workers;worker++)for(unsigned slot=0;slot<ResidentSlotsPerWorker;slot++){
			const unsigned index=worker*SlotsPerWorker+slot;
			cacheValid[index/64]|=UINT64_C(1)<<(index%64);
		}
		cachePosition=position+1;cacheSlot^=1;cacheBound=true;
		phase=Ready;return true;
	}
	void Fault()
	{
		if(phase!=Closed){Invalidate();state.Fault();phase=Poisoned;}
	}
	bool Unsafe() const{return phase==Poisoned || phase==Token || phase==Layer
		|| phase==PendingModelPublication || state.Unsafe();}
	bool Close()
	{
		if(phase!=Ready || !state.Close())return false;
		Invalidate();phase=Closed;return true;
	}
};
#endif
