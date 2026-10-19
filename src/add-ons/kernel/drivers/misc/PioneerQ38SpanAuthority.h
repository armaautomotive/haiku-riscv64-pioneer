/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_AUTHORITY_H
#define PIONEER_Q38_SPAN_AUTHORITY_H
#include "PioneerResidentBanks.h"
#include "PioneerQ38SpanShape.h"
// OFFLINE research,NOT a public request/capability. A future controller
// must construct this afresh from its own bound bank owner and bitmap,
// before ANY IO. No user route,address or initialized-page claim is used.
struct PioneerQ38SpanExtent {
	uint32_t bank=0,address=0,route=0,index=0,pages=0,bytes=0;
};
static inline bool PioneerQ38SpanAuthorize(const PioneerResidentBanks& banks,const tt_dram_samples_result& baseline,
	const uint8_t* initialized,uint32_t initializedPages,unsigned bank,uint64_t offset,unsigned bytes,PioneerQ38SpanExtent& result)
{
	result={};
	if (!initialized || initializedPages!=PioneerResidentBanks::Pages || bank>=7
		|| !bytes || bytes>TT_Q38_SPAN_HOST_BYTES || (bytes&4095) || (offset&4095)
		|| offset>=PioneerResidentBanks::Capacity || bytes>PioneerResidentBanks::Capacity-offset
		|| !banks.Matches(baseline)) return false;
	PioneerQ38SpanExtent stage; stage.bank=bank; stage.pages=bytes/4096; stage.bytes=bytes;
	for (unsigned p=0;p<stage.pages;p++) {
		uint32_t address=0,route=0,index=0;
		// The EXISTING <=4-page bank API is intentionally NOT widened.
		// Prove every page and continuity through its existing one-page API.
		if (!banks.Page(baseline,bank,offset+uint64_t(p)*4096,address,route,index)
			|| route!=PioneerQ38SpanRoute(bank) || index>=initializedPages || initialized[index]) return false;
		if (!p) { stage.address=address; stage.route=route; stage.index=index; }
		else if (address!=stage.address+p*4096 || route!=stage.route || index!=stage.index+p) return false;
	}
	result=stage; return true;
}
#endif
