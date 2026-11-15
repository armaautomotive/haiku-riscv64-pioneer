/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SHAPE_H
#include "PioneerQ38GdnRendezvousShape.h"
#include "PioneerQ38GdnTaskPlacement.h"

// Experimental DATA-shape checks for layer-dependent input/state/gating
// routes. They normalize into the unchanged legacy validators only after
// authenticating remapped peers. No launch or owner authority is added.
static inline bool PioneerQ38ResidentGdnRedistributedShape(const uint32_t* d)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;return false;
#else
	if(!d || d[9]>=PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
	uint32_t ordinal=0;
	if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(d[10],ordinal))return false;
	PioneerQ38GdnTaskPlacement::Route route={};
	return PioneerQ38GdnTaskPlacement::Map(ordinal,d[9],route)
		&& PioneerQ38ResidentGdnShapePeer(d,route.peer);
#endif
}

static inline bool PioneerQ38GdnRedistributedInputShape(const uint32_t* d)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;return false;
#else
	if(!d || d[7]>=PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
	uint32_t ordinal=0;
	if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(d[5],ordinal))return false;
	PioneerQ38GdnTaskPlacement::Route route={};
	return PioneerQ38GdnTaskPlacement::Map(ordinal,d[7],route)
		&& PioneerQ38GdnInputShapePeer(d,route.peer);
#endif
}

static inline bool PioneerQ38GdnRedistributedPairing(const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)input;(void)state;return false;
#else
	if(!PioneerQ38GdnRedistributedInputShape(input)
		|| !PioneerQ38ResidentGdnRedistributedShape(state))return false;
	if(input[5]!=state[10] || input[6]!=state[8] || input[7]!=state[9]
		|| input[9]!=state[15] || input[40]!=state[40] || state[41])return false;
	for(unsigned i=16;i<40;i++)if(input[i]!=state[i])return false;
	for(unsigned side=0;side<2;side++){
		unsigned bank=state[4+side*2],at=state[5+side*2];
		if(!PioneerQ38GdnRecurrentSeparate(bank,at,TT_Q38_RGDN_STATE_BYTES,
			input[3],input[4],67108864)
			|| !PioneerQ38GdnRecurrentSeparate(bank,at,TT_Q38_RGDN_STATE_BYTES,
				input[11],input[12],64)
			|| !PioneerQ38GdnRecurrentSeparate(bank,at,TT_Q38_RGDN_STATE_BYTES,
				input[14],input[15],64))return false;
	}
	return true;
#endif
}

static inline bool PioneerQ38GdnRedistributedGatedBaseShape(const uint32_t* d,
	const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;return false;
#else
	if(!d || d[7]>=PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
	uint32_t ordinal=0;
	PioneerQ38GdnTaskPlacement::Route local={},sibling={};
	if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(d[5],ordinal)
		|| !PioneerQ38GdnTaskPlacement::Map(ordinal,d[7],local)
		|| !PioneerQ38GdnTaskPlacement::Map(ordinal,d[7]^1,sibling)
		|| d[9]!=local.peer || d[15]!=sibling.peer)return false;
	return PioneerQ38GdnGatedBaseShape(d,input,state,local.peer,local.peer,
		local.peer,sibling.peer);
#endif
}

static inline bool PioneerQ38GdnRedistributedGatedShape(const uint32_t* d,const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;return false;
#else
	return d && d[0]==TT_Q38_GDN_GATED_MARKER
		&& d[51]==TT_Q38_GDN_GATED_POLLS
		&& PioneerQ38GdnRedistributedGatedBaseShape(d,input,state)
		&& [&]() { for(unsigned i=52;i<64;i++)if(d[i])return false;return true; }();
#endif
}

static inline bool PioneerQ38GdnRedistributedRendezvousShape(const uint32_t* d,const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;return false;
#else
	if(!d || d[0]!=TT_Q38_GDN_RENDEZVOUS_MARKER
		|| d[51]!=TT_Q38_GDN_RENDEZVOUS_POLLS
		|| d[52]!=uint32_t(PioneerQ38GdnRendezvousBudget())
		|| d[53]!=uint32_t(PioneerQ38GdnRendezvousBudget()>>32)
		|| d[54]!=4 || d[55]!=TT_Q38_GDN_RENDEZVOUS_CLOCK)return false;
	for(unsigned i=56;i<64;i++)if(d[i])return false;
	return PioneerQ38GdnRedistributedGatedBaseShape(d,input,state);
#endif
}
#endif
