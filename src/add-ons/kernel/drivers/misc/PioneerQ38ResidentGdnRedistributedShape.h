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
	if(!PioneerQ38GdnTaskPlacement::Map(ordinal,d[9],route) || d[15]!=route.peer)return false;
	uint32_t legacy[64];for(unsigned i=0;i<64;i++)legacy[i]=d[i];
	legacy[15]=PioneerQ38ResidentGdnPeer(d[9]);
	return PioneerQ38ResidentGdnShape(legacy);
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
	if(!PioneerQ38GdnTaskPlacement::Map(ordinal,d[7],route) || d[9]!=route.peer)return false;
	uint32_t legacy[64];for(unsigned i=0;i<64;i++)legacy[i]=d[i];
	legacy[9]=PioneerQ38ConvPeer(d[10]);
	return PioneerQ38GdnInputShape(legacy);
#endif
}

static inline bool PioneerQ38GdnRedistributedPairing(const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)input;(void)state;return false;
#else
	if(!PioneerQ38GdnRedistributedInputShape(input)
		|| !PioneerQ38ResidentGdnRedistributedShape(state))return false;
	uint32_t legacyInput[64],legacyState[64];
	for(unsigned i=0;i<64;i++){legacyInput[i]=input[i];legacyState[i]=state[i];}
	legacyInput[9]=PioneerQ38ConvPeer(legacyInput[10]);
	legacyState[15]=PioneerQ38ResidentGdnPeer(legacyState[9]);
	return PioneerQ38GdnRecurrentPairing(legacyInput,legacyState);
#endif
}

static inline bool PioneerQ38GdnRedistributedGatedShape(const uint32_t* d,const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;return false;
#else
	if(!d || !PioneerQ38GdnRedistributedPairing(input,state)
		|| d[7]>=PioneerQ38GdnTaskPlacement::LogicalWorkers || !d[14]
		|| d[14]>PioneerQ38GdnTaskPlacement::LogicalWorkers)return false;
	uint32_t ordinal=0;
	if(!PioneerQ38GdnTaskPlacement::LayerOrdinal(d[5],ordinal))return false;
	PioneerQ38GdnTaskPlacement::Route local={},sibling={};
	if(!PioneerQ38GdnTaskPlacement::Map(ordinal,d[7],local)
		|| !PioneerQ38GdnTaskPlacement::Map(ordinal,d[14]-1,sibling)
		|| d[9]!=local.peer || d[15]!=sibling.peer)return false;
	uint32_t legacyD[64],legacyInput[64],legacyState[64];
	for(unsigned i=0;i<64;i++){legacyD[i]=d[i];legacyInput[i]=input[i];legacyState[i]=state[i];}
	legacyD[9]=PioneerQ38ConvPeer(legacyD[10]);
	legacyD[15]=PioneerQ38ConvPeer(legacyD[14]);
	legacyInput[9]=PioneerQ38ConvPeer(legacyInput[10]);
	legacyState[15]=PioneerQ38ResidentGdnPeer(legacyState[9]);
	return PioneerQ38GdnGatedShape(legacyD,legacyInput,legacyState);
#endif
}

static inline bool PioneerQ38GdnRedistributedRendezvousShape(const uint32_t* d,const uint32_t* input,const uint32_t* state)
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	(void)d;(void)input;(void)state;return false;
#else
	if(!d)return false;
	uint32_t gated[64];for(unsigned i=0;i<64;i++)gated[i]=d[i];
	gated[0]=TT_Q38_GDN_GATED_MARKER;gated[51]=TT_Q38_GDN_GATED_POLLS;
	for(unsigned i=52;i<56;i++)gated[i]=0;
	if(!PioneerQ38GdnRedistributedGatedShape(gated,input,state))return false;
	uint32_t legacyD[64],legacyInput[64],legacyState[64];
	for(unsigned i=0;i<64;i++){legacyD[i]=d[i];legacyInput[i]=input[i];legacyState[i]=state[i];}
	legacyD[9]=PioneerQ38ConvPeer(legacyD[10]);
	legacyD[15]=PioneerQ38ConvPeer(legacyD[14]);
	legacyInput[9]=PioneerQ38ConvPeer(legacyInput[10]);
	legacyState[15]=PioneerQ38ResidentGdnPeer(legacyState[9]);
	return PioneerQ38GdnRendezvousShape(legacyD,legacyInput,legacyState);
#endif
}
#endif
