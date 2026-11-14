/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_REDISTRIBUTED_GDN_PLAN_H
#define PIONEER_Q38_REDISTRIBUTED_GDN_PLAN_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnTaskPlacement.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnGatedPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRedistributedServiceShape.h"

// Immutable candidate metadata for 96 logical GDN tasks routed over 119
// physical workers, keyed by physical worker. DATA only: no launch, state
// initialization, source ownership, native selection, or Work authority.
class PioneerQ38RedistributedGdnPlan {
	static constexpr unsigned RecordWords=192;
	static constexpr unsigned PhysicalWords=PioneerQ38GdnTaskPlacement::PhysicalWorkers*RecordWords;
	const PioneerQ38CanonicalModelPlan* canonical=nullptr;
	uint32_t header[32]={};
	uint32_t records[PhysicalWords]={};
	bool attempted=false,ready=false;
	static bool State(const PioneerQ38CanonicalModelPlan& plan,const uint32_t* input,
		unsigned slot,uint32_t* d)
	{
		if(!input || slot>=2 || !PioneerQ38GdnInputShape(input))return false;
		for(unsigned i=0;i<64;i++)d[i]=0;
		d[0]=TT_Q38_RGDN_MARKER;d[1]=1;d[2]=128;d[3]=64;d[8]=input[6];
		d[9]=input[7];d[10]=input[5];d[11]=slot;d[14]=32768;d[15]=input[9];d[40]=input[40];
		for(unsigned i=16;i<40;i++)d[i]=input[i];
		for(unsigned side=0;side<2;side++){
			unsigned found=0;const unsigned head=input[7]/2;
			for(unsigned i=0;i<plan.RecurrentCount();i++){
				const auto& region=plan.RecurrentRegions()[i];
				if(region.layer!=input[5] || region.slot!=(slot^side) || head<region.firstHead
					|| head-region.firstHead>=region.heads)continue;
				if(++found!=1 || region.bank>=7 || !region.heads)return false;
				const uint64_t offset=uint64_t(region.offset)+(head-region.firstHead)*UINT64_C(65536)
					+(input[7]&1)*32768;
				if(offset>plan.Capacity || 32768>plan.Capacity-offset)return false;
				d[4+side*2]=region.bank;d[5+side*2]=plan.Base+uint32_t(offset);
				d[12+side]=PioneerQ38ProjectionRoute(region.bank);
			}
			if(found!=1)return false;
		}
		return PioneerQ38GdnRecurrentShape(input,d);
	}
	static bool Record(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,
		unsigned position,unsigned bank,unsigned slot,unsigned phase,
		const PioneerQ38GdnIdentity& identity,const uint32_t* sources,
		unsigned logical,uint32_t* output,unsigned& physical)
	{
		uint32_t ordinal=0;
		if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(layer,ordinal))return false;
		PioneerQ38GdnTaskPlacement::Route route={},sibling={};
		if(!PioneerQ38GdnTaskPlacement::Map(ordinal,logical,route)
			|| !PioneerQ38GdnTaskPlacement::Map(ordinal,logical^1,sibling))return false;
		PioneerQ38GdnInputPlan inputPlan;
		if(!inputPlan.Build(plan,layer,logical,position,bank,identity,
			sources+route.physicalWorker*16,phase+1))return false;
		uint32_t input[64],state[64],gated[64];
		for(unsigned i=0;i<64;i++)input[i]=inputPlan.Descriptor()[i];
		if(!State(plan,input,slot,state))return false;
		PioneerQ38GdnGatedPlan gatedPlan;
		if(!gatedPlan.Build(plan,input,state))return false;
		for(unsigned i=0;i<64;i++)gated[i]=gatedPlan.Descriptor()[i];
		input[9]=route.peer;state[15]=route.peer;gated[9]=route.peer;gated[15]=sibling.peer;
		gated[0]=TT_Q38_GDN_RENDEZVOUS_MARKER;gated[51]=TT_Q38_GDN_RENDEZVOUS_POLLS;
		gated[52]=uint32_t(PioneerQ38GdnRendezvousBudget());
		gated[53]=uint32_t(PioneerQ38GdnRendezvousBudget()>>32);
		gated[54]=4;gated[55]=TT_Q38_GDN_RENDEZVOUS_CLOCK;
		if(!PioneerQ38GdnRedistributedPairing(input,state)
			|| !PioneerQ38GdnRedistributedRendezvousShape(gated,input,state))return false;
		for(unsigned i=0;i<64;i++){
			output[i]=input[i];output[64+i]=state[i];output[128+i]=gated[i];
		}
		physical=route.physicalWorker;
		return true;
	}
	bool Inputs(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,
		unsigned bank,unsigned slot,unsigned phase,const PioneerQ38GdnIdentity& identity,
		const uint32_t* sources,uint32_t* destination,bool compare)const
	{
		bool active[PioneerQ38GdnTaskPlacement::PhysicalWorkers]={};unsigned count=0;
		for(unsigned logical=0;logical<PioneerQ38GdnTaskPlacement::LogicalWorkers;logical++){
			uint32_t record[RecordWords];unsigned physical=0;
			if(!Record(plan,layer,position,bank,slot,phase,identity,sources,logical,record,physical)
				|| !physical || physical>PioneerQ38GdnTaskPlacement::PhysicalWorkers || active[physical-1])return false;
			active[physical-1]=true;count++;
			const unsigned offset=(physical-1)*RecordWords;
			if(compare){for(unsigned i=0;i<RecordWords;i++)if(records[offset+i]!=record[i])return false;}
			else for(unsigned i=0;i<RecordWords;i++)destination[offset+i]=record[i];
		}
		if(count!=PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
		for(unsigned physical=1;physical<=PioneerQ38GdnTaskPlacement::PhysicalWorkers;physical++){
			if(active[physical-1])continue;
			const unsigned offset=(physical-1)*RecordWords;
			for(unsigned i=0;i<RecordWords;i++){
				if(compare){if(records[offset+i])return false;}
				else destination[offset+i]=0;
			}
		}
		return true;
	}
public:
	PioneerQ38RedistributedGdnPlan()=default;
	PioneerQ38RedistributedGdnPlan(const PioneerQ38RedistributedGdnPlan&)=delete;
	PioneerQ38RedistributedGdnPlan& operator=(const PioneerQ38RedistributedGdnPlan&)=delete;
	bool Build(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,
		unsigned bank,unsigned slot,unsigned phase,unsigned request,
		const PioneerQ38GdnIdentity& identity,const uint32_t* sources)
	{
		if(attempted)return false;attempted=true;
		if(!plan.Ready() || layer>=64 || layer%4==3 || position>=8192 || bank>=7
			|| slot>=2 || !phase || phase>=0x7ffffffa || !request || request>=0x7fffffff
			|| !identity.Valid() || !sources)return false;
		for(unsigned w=0;w<120;w++)if(!sources[w*16])return false;
		canonical=&plan;
		const unsigned fields[]={0x31444752,1,PhysicalWords,layer,position,bank,slot,phase,request,
			PioneerQ38GdnTaskPlacement::LogicalWorkers,PioneerQ38GdnTaskPlacement::PhysicalWorkers,
			PioneerQ38GdnTaskPlacement::RecurrentLayers};
		for(unsigned i=0;i<12;i++)header[i]=fields[i];
		const uint64_t values[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
		for(unsigned i=0;i<4;i++){header[16+i*2]=uint32_t(values[i]);header[17+i*2]=uint32_t(values[i]>>32);}
		if(!Inputs(plan,layer,position,bank,slot,phase,identity,sources,records,false))return false;
		ready=true;return Matches(plan,layer,position,bank,slot,phase,request,identity,sources);
	}
	bool Matches(const PioneerQ38CanonicalModelPlan& plan,unsigned layer,unsigned position,
		unsigned bank,unsigned slot,unsigned phase,unsigned request,
		const PioneerQ38GdnIdentity& identity,const uint32_t* sources)const
	{
		if(!ready || canonical!=&plan || !plan.Ready() || !sources || header[0]!=0x31444752
			|| header[1]!=1 || header[2]!=PhysicalWords || header[3]!=layer || header[4]!=position
			|| header[5]!=bank || header[6]!=slot || header[7]!=phase || header[8]!=request
			|| header[9]!=PioneerQ38GdnTaskPlacement::LogicalWorkers
			|| header[10]!=PioneerQ38GdnTaskPlacement::PhysicalWorkers
			|| header[11]!=PioneerQ38GdnTaskPlacement::RecurrentLayers)return false;
		const uint64_t values[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
		for(unsigned i=0;i<4;i++)if(header[16+i*2]!=uint32_t(values[i])
			|| header[17+i*2]!=uint32_t(values[i]>>32))return false;
		for(unsigned i=12;i<16;i++)if(header[i])return false;
		for(unsigned i=24;i<32;i++)if(header[i])return false;
		for(unsigned w=0;w<120;w++)if(!sources[w*16])return false;
		return Inputs(plan,layer,position,bank,slot,phase,identity,sources,
		nullptr,true);
	}
	const uint32_t* Header()const{return ready?header:nullptr;}
	const uint32_t* Worker(unsigned physicalWorker)const{
		return ready && physicalWorker && physicalWorker<=PioneerQ38GdnTaskPlacement::PhysicalWorkers
			?records+(physicalWorker-1)*RecordWords:nullptr;
	}
	bool Active(unsigned physicalWorker)const{
		if(!Worker(physicalWorker))return false;
		for(unsigned i=0;i<RecordWords;i++)if(records[(physicalWorker-1)*RecordWords+i])return true;
		return false;
	}
};
static_assert(sizeof(PioneerQ38RedistributedGdnPlan)>90000 && sizeof(PioneerQ38RedistributedGdnPlan)<100000,
	"119 physical GDN slots with 96 layer-routed tasks are HEAP DATA");
#endif
