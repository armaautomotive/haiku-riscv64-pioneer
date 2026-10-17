/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FAST_SOFTMAX_H
#define PIONEER_FAST_SOFTMAX_H
#include "PioneerGraphMath.h"
// OFFLINE numerical candidate,not bit-identical to the two-float reference.
// Stable softmax ONLY:x<=0,finite. Same -80 cutoff and exact exp(0)=1.
// Requires unchanged full-model/native quality and natural-request gates.
static inline float PioneerFastSoftmaxExp(float value)
{
	uint32_t absolute=PioneerAttentionBits(value)&0x7fffffff;
	if (value>0 || absolute>=0x7f800000) return PioneerAttentionFloat(0x7fc00000);
	return PioneerGraphExpFast(value);
}
#endif
