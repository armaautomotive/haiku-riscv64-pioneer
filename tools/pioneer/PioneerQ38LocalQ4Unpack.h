/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LOCAL_Q4_UNPACK_H
#define PIONEER_Q38_LOCAL_Q4_UNPACK_H
#include "PioneerQ38LocalQ4.h"
// OFFLINE ONLY. Original2304B NoC tile90000 and metadata91000/97000 remain
// unchanged. Caller has expanded THIS block to saved private92000..93100.
// Same native SM7 SrcA conversion as the separate predecoded-Q4 study;
// all values0..15 are exact. No doubled model/VRAM/host wire format.
#ifdef PIONEER_ATTENTION_FREESTANDING
#define Q38_LOCAL_HELPER(name) __attribute__((noinline,optimize("Os"),section(".text.pioneer_q38_local_q4_" #name)))
#else
#define Q38_LOCAL_HELPER(name)
#endif
template<class E> static Q38_LOCAL_HELPER(setup) void PioneerQ38LocalQ4Setup(E& e)
{
	e.Drain();e.Config(0,0x423e);e.Config(1,0xa03c0000);e.Config(73,3);
	e.Op(0x8a000002);e.Drain();
}
template<class E> static Q38_LOCAL_HELPER(unpack) bool PioneerQ38LocalQ4Unpack(E& e,unsigned panel)
{
	if(panel>=16)return false;
	e.Drain();e.Config(76,(0x92100+panel*256-16)/16);
	e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
	e.Op(0x420000c1);e.Drain();return true;
}
template<class E> static Q38_LOCAL_HELPER(restore) void PioneerQ38LocalQ4Restore(E& e)
{
	// Caller first drains the final deferred affine store. Preserve the
	// original metadata/SFPU contract BEFORE the next block or group export.
	e.Config(0,0x4231);e.Config(1,0xa03c8000);e.Config(73,0x13);e.Drain();
}
#undef Q38_LOCAL_HELPER
#endif
