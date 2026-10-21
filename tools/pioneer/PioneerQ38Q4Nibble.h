/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_NIBBLE_H
#define PIONEER_Q38_Q4_NIBBLE_H
#include "PioneerSfpuDivide.h"
// OFFLINE arithmetic/scheduling prerequisite, NOT a native driver selector.
// Q4's existing unsigned raw UNPACK/SFPLOAD supplies EXACTLY 0..255 in
// LReg0. Keep the existing unpack, SrcA handoff, Dst windows and stores.
// Low nibble: load mask ONCE outside REPLAY, then one AND per fresh chunk.
// High nibble: a logical right shift alone is exact for that byte domain.
// LReg4 survives only this one panel's eight register-only replay chunks;
// never reuse it across panels/jobs, nor assume an earlier panel set it.
template<class E> static bool PioneerQ38Q4NibbleSetup(E& e,unsigned panel)
{
	if(panel>=16)return false;
	if(!(panel&2))PioneerSfpuLoad32(e,4,15);
	return true;
}
template<class E> static bool PioneerQ38Q4NibbleDecode(E& e,unsigned panel)
{
	if(panel>=16)return false;
	if(panel&2)PioneerSfpuShift(e,0,-4);
	else PioneerSfpuAnd(e,0,0,4);
	return true;
}
#endif
