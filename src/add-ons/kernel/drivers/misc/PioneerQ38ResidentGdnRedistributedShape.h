/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_REDISTRIBUTED_SHAPE_H
#include "PioneerQ38ResidentGdnShape.h"
#include "PioneerQ38GdnTaskPlacement.h"

// Experimental DATA-shape check for the layer-dependent task route. It
// validates every legacy state descriptor invariant after separately
// authenticating the candidate peer. No launch or owner authority is added.
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
#endif
