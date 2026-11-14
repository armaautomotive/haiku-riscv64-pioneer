/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SERVICE_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SERVICE_SHAPE_H
#include "PioneerQ38ResidentGdnServiceShape.h"
#include "PioneerQ38ResidentGdnRedistributedShape.h"

// Candidate physical service peer + logical half-head job proof. All
// descriptor routes are checked before job fields; legacy Work stays fixed.
static inline bool PioneerQ38ResidentGdnRedistributedServiceJob(const uint32_t* j,
	const uint32_t* d,const uint32_t* input,const uint32_t* state,
	const uint32_t* source,unsigned worker,unsigned self)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)j;(void)d;(void)input;(void)state;(void)source;(void)worker;(void)self;
	return false;
#else
	if(!j || !d || !input || !state || !source || !worker
		|| worker>PioneerQ38GdnTaskPlacement::PhysicalWorkers
		|| !PioneerQ38GdnRedistributedRendezvousShape(d,input,state))return false;
	uint32_t ordinal=0;
	if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(state[10],ordinal)
		|| state[9]>=PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
	PioneerQ38GdnTaskPlacement::Route route={};
	if(!PioneerQ38GdnTaskPlacement::Map(ordinal,state[9],route)
		|| route.physicalWorker!=worker || j[11]!=worker || route.actualLayer!=state[10]
		|| d[7]!=state[9] || d[9]!=route.peer || input[9]!=route.peer
		|| state[15]!=route.peer || self!=route.peer)return false;
	if(j[0]!=TT_Q38_RESIDENT_GDN_SERVICE_JOB || j[1]!=64 || j[2]!=768
		|| j[3]!=d[6] || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER
		|| !j[12] || j[12]>=0x7fffffff || j[12]>j[3]
		|| j[13]!=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION || j[14] || j[15])return false;
	for(unsigned i=4;i<10;i++)if(j[i])return false;
	for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
	return true;
#endif
}

static inline bool PioneerQ38ResidentGdnRedistributedServiceSummaries(
	const uint32_t* d,const uint32_t* input,const uint32_t* state,
	const uint32_t* first,const uint32_t* recurrent,const uint32_t* gated,
	const uint32_t* clock,const uint32_t* before,const uint32_t* after)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;(void)first;(void)recurrent;(void)gated;
	(void)clock;(void)before;(void)after;return false;
#else
	if(!d || !input || !state || !first || !recurrent || !gated || !clock
		|| !before || !after || !PioneerQ38GdnRedistributedRendezvousShape(d,input,state)
		|| recurrent[28]!=state[15] || gated[27]!=d[15] || !gated[5]
		|| gated[5]>TT_Q38_GDN_RENDEZVOUS_POLLS)return false;
	if(before[2]!=before[6] || after[2]!=after[6])return false;
	const unsigned firstHeader[]={7,24,0,0,322,1536,input[6],TT_Q38_GDN_INPUT_MARKER};
	const unsigned recurrentHeader[]={2,512,2,512,64,32768,state[8],TT_Q38_RGDN_MARKER};
	const unsigned gatedHeader[]={gated[5]+3,gated[5]+20,1,4,64,gated[5],d[6],TT_Q38_GDN_RENDEZVOUS_MARKER};
	const unsigned firstDelta[]={7,24,7,7,0,0,7,0};
	const unsigned recurrentDelta[]={2,512,4,2,2,512,4,2};
	const unsigned gatedDelta[]={gated[5]+3,gated[5]+20,gated[5]+4,gated[5]+3,1,4,gated[5]+4,1};
	for(unsigned i=0;i<8;i++){
		if(first[i]!=firstHeader[i] || first[8+i]!=before[i]
			|| uint32_t(first[16+i]-first[8+i])!=firstDelta[i]
			|| recurrent[i]!=recurrentHeader[i] || recurrent[8+i]!=first[16+i]
			|| uint32_t(recurrent[16+i]-recurrent[8+i])!=recurrentDelta[i]
			|| gated[i]!=gatedHeader[i] || gated[8+i]!=recurrent[16+i]
			|| uint32_t(gated[16+i]-gated[8+i])!=gatedDelta[i]
			|| after[i]!=gated[16+i])return false;
	}
	const unsigned firstTail[]={input[7],input[5],input[40],input[10],input[11],input[14],input[42],input[44]};
	const unsigned recurrentTail[]={state[9],state[10],state[11],state[11]^1,state[15],1,state[5],state[7]};
	const unsigned gatedTail[]={d[7],d[5],d[40],d[15],d[4]+d[43],d[3],d[11],d[12]};
	for(unsigned i=0;i<8;i++)if(first[24+i]!=firstTail[i]
		|| recurrent[24+i]!=recurrentTail[i] || gated[24+i]!=gatedTail[i])return false;
	return PioneerQ38GdnRendezvousClockReceipt(clock,d,gated[5]);
#endif
}

static inline bool PioneerQ38ResidentGdnRedistributedServiceProof(
	const uint32_t* p,const uint32_t* j,const uint32_t* d,const uint32_t* input,
	const uint32_t* state,const uint32_t* source,unsigned worker,unsigned self,
	const uint32_t* before,const uint32_t* after,const uint32_t* first,
	const uint32_t* recurrent,const uint32_t* gated,const uint32_t* clock)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)p;(void)j;(void)d;(void)input;(void)state;(void)source;(void)worker;(void)self;
	(void)before;(void)after;(void)first;(void)recurrent;(void)gated;(void)clock;return false;
#else
	if(!p || p[18]!=worker || p[22]!=self || p[31]!=self
		|| !PioneerQ38ResidentGdnRedistributedServiceJob(j,d,input,state,source,worker,self)
		|| !PioneerQ38ResidentGdnRedistributedServiceSummaries(d,input,state,first,
			recurrent,gated,clock,before,after))return false;
	for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i])return false;
	const unsigned command[]={0x8ea00,0,self,d[4]+d[43],0,d[8],0,0x212,256,0};
	for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
	if(p[16]!=64 || p[17]!=j[12] || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
		|| p[30]!=source[0] || p[48]!=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION
		|| p[49]!=d[6] || p[50]!=d[5] || p[51]!=d[7] || p[52]!=d[40]
		|| p[53]!=state[11] || p[62]!=TT_Q38_GDN_RENDEZVOUS_MARKER
		|| p[63]!=gated[5])return false;
	for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
	for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
	return true;
#endif
}

#endif
