/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_ROPE_H
#define PIONEER_GRAPH_ROPE_H
#include "PioneerAttentionMath.h"
// OFFLINE decoder prerequisite. The exact pinned-CPU cos/sin table may be
// computed once per position/model,not learned CPU attention arithmetic.
// Fixed128-dimensional NEOX rotation only;no YaRN/MRoPE/partial rotation.
// Both input/output may be the SAME vector. No partial-overlap authority.
static inline bool PioneerGraphRopeNeox(unsigned n,const float* input,const float* table,float* output)
{
	if ((n!=1024 && n!=2048) || !input || !table || !output) return false;
	// Explicit normal bounded domain;do not silently accept arithmetic that
	// would depend on unqualified BRISC denormal multiplication behavior.
	for (unsigned i=0;i<128;i++) { uint32_t a=PioneerAttentionBits(table[i])&0x7fffffff; if (a>0x3f800000 || (a && a<0x26800000)) return false; }
	for (unsigned i=0;i<n;i++) { uint32_t a=PioneerAttentionBits(input[i])&0x7fffffff; if (a>0x477fe000 || (a && a<0x26800000)) return false; }
	for (unsigned head=0;head<n;head+=128) for (unsigned i=0;i<64;i++) {
		float a=input[head+i],b=input[head+64+i],c=table[2*i],s=table[2*i+1];
		// Keep the pinned unfused order;no hardware partially-fused SFPMAD.
		float x=a*c-b*s,y=a*s+b*c;
		if ((PioneerAttentionBits(x)&0x7f800000)==0x7f800000 || (PioneerAttentionBits(y)&0x7f800000)==0x7f800000) return false;
		output[head+i]=x; output[head+64+i]=y;
	}
	return true;
}
#endif
