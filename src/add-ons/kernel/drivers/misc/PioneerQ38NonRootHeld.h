/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NON_ROOT_HELD_H
#define PIONEER_Q38_NON_ROOT_HELD_H
#include "BlackholeDramSamplesProbe.h"
// OFFLINE adapter prerequisite ONLY,no native operation selects this helper.
// The future IO adapter must bind its SAME private bank/context owner and
// already-owned read-only reset/TLB views. Never call the old ALL120-held
// audit while the zeroing root is intentionally running. No reset/NoC/PCI
// command/data write capability is needed. On failure stop immediately;
// do NOT restore even the root view after uncertainty.
template<class IO> bool PioneerQ38NonRootHeld(IO& io,const tt_dram_samples_result& proof)
{
	static_assert(TT_DRAM_SOURCE_WORDS==16,"Fixed qualified source-view width");
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	if (proof.version!=1 || proof.routes_verified!=7 || proof.recovery_required
		|| !io.NonRootHeldOwner() || !io.NonRootHeldMappingStable()) return false; // CPU ownership gate
	uint32_t physicalMask=0;
	for (unsigned x:columns) {
		unsigned physical=BlackholeDramSourceX(proof.source,x);
		if (physical>16 || (proof.source[13]&(1U<<x)) || (physicalMask&(1U<<physical))) return false;
		physicalMask|=1U<<physical;
	}
	if (physicalMask!=0x1f87e) return false;
	for (unsigned y=2;y<=11;y++) if ((proof.source[14]&(1U<<y)) || BlackholeDramSourceY(proof.source,y)!=y) return false;
	auto sources=[&]() {
		if (!io.NonRootHeldOwner() || !io.NonRootHeldMappingStable()) return false;
		for (unsigned word=0;word<TT_DRAM_SOURCE_WORDS;word++) if (io.SourceRead(word)!=proof.source[word]) return false;
		return true;
	};
	auto select=[&](unsigned x,unsigned y) {
		if (!io.NonRootHeldMappingStable()) return false;
		for (unsigned word=0;word<3;word++) io.DramTlbWrite(1,word,BlackholeDramFixedTlbWord(0xffa00000,x,y,true,word));
		for (unsigned word=0;word<3;word++) if (io.DramTlbRead(1,word)!=BlackholeDramFixedTlbWord(0xffa00000,x,y,true,word)) return false;
		return true;
	};
	if (!sources()) return false;
	for (unsigned x:columns) for (unsigned y=2;y<=11;y++) {
		if (x==1 && y==2) continue; // the owned root may be RUNNING;NEVER require/reset/hold it here
		if (!select(x,y) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) return false;
	}
	if (!sources()) return false;
	return select(1,2); // known-clean view restoration ONLY;no root reset read/write
}
#endif
