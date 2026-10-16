/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_OUTPUT_BUFFER_H
#define PIONEER_OUTPUT_BUFFER_H
#include "PioneerHostPage.h"
// Preparation/ownership policy. A separate CPU-only probe can qualify the
// native allocation. The opt-in bulk upload controller separately qualifies
// this owner before any transfer; the allocation probe never issues DMA.
// A transfer controller must quarantine BOTH this buffer and the
// existing input page after uncertainty, not just one of them.
enum { TT_OUTPUT_BUFFER_BYTES=16384,TT_OUTPUT_BUFFER_PAGES=4,TT_OUTPUT_BUFFER_WORDS=4096 };
enum {
	TT_OUTPUT_BUFFER_REFUSED=0,TT_OUTPUT_BUFFER_READY=1,TT_OUTPUT_BUFFER_ALLOC_FAILED=2,
	TT_OUTPUT_BUFFER_BAD_MAP=3,TT_OUTPUT_BUFFER_CACHE_FAILED=4,TT_OUTPUT_BUFFER_PATTERN_FAILED=5,
	TT_OUTPUT_BUFFER_MAP_CHANGED=6,TT_OUTPUT_BUFFER_RELEASE_FAILED=7
};
struct PioneerOutputBufferLease {
	bool allocated=false,ready=false,memory_type_set=false,released=false,quarantined=false,pending=false;
	uint32_t state=TT_OUTPUT_BUFFER_REFUSED,patterns=0,mismatches=0;
	uint64_t physical=0;
};
static inline bool PioneerOutputBufferAddress(uint64_t physical,uint64_t bytes)
{
	return bytes==TT_OUTPUT_BUFFER_BYTES && physical>=UINT64_C(0x100000000)
		&& !(physical&4095) && physical<UINT64_C(0x1000000000000)
		&& bytes<=UINT64_C(0x1000000000000)-physical;
}
static inline bool PioneerOutputBufferDistinct(uint64_t physical,const tt_host_page_result& input)
{
	return input.version==1 && input.state==TT_HOST_PAGE_READY && input.allocated
		&& input.memory_type_set && input.map_stable && !input.released
		&& PioneerHostPageAddress(input.physical,input.bytes)
		&& PioneerOutputBufferAddress(physical,TT_OUTPUT_BUFFER_BYTES)
		&& (physical+TT_OUTPUT_BUFFER_BYTES<=input.physical || input.physical+input.bytes<=physical);
}
template<class IO>
bool PioneerOutputBufferMapStable(IO& io,const PioneerOutputBufferLease& out,const tt_host_page_result& input)
{
	uint64_t physical=0,bytes=0;
	if (!out.allocated || out.released || out.quarantined || io.OutputBufferMap(physical,bytes)!=0
		|| physical!=out.physical || !PioneerOutputBufferAddress(physical,bytes) || !PioneerOutputBufferDistinct(physical,input)) return false;
	for (unsigned page=0;page<TT_OUTPUT_BUFFER_PAGES;page++) {
		uint64_t address=0,length=0;
		if (io.OutputBufferPageMap(page,address,length)!=0 || length<4096 || address!=physical+page*4096) return false;
	}
	return true;
}
template<class IO>
bool PioneerPrepareOutputBuffer(IO& io,const tt_host_page_result& input,PioneerOutputBufferLease& out)
{
	if (out.allocated || out.released || out.quarantined || out.pending) return false;
	// Refuse an unqualified input owner before allocating any second area.
	if (input.version!=1 || input.state!=TT_HOST_PAGE_READY || !input.allocated || !input.memory_type_set
		|| !input.map_stable || input.released || !PioneerHostPageAddress(input.physical,input.bytes)) return false;
	if (io.AllocateOutputBuffer()!=0) { out.state=TT_OUTPUT_BUFFER_ALLOC_FAILED; return false; }
	out.allocated=true; out.state=TT_OUTPUT_BUFFER_BAD_MAP;
	uint64_t physical=0,bytes=0;
	if (io.OutputBufferMap(physical,bytes)!=0 || !PioneerOutputBufferAddress(physical,bytes)
		|| !PioneerOutputBufferDistinct(physical,input)) return false;
	out.physical=physical;
	if (!PioneerOutputBufferMapStable(io,out,input)) return false;
	out.state=TT_OUTPUT_BUFFER_CACHE_FAILED;
	if (io.SetOutputBufferNonCached()!=0) return false;
	out.memory_type_set=true; out.state=TT_OUTPUT_BUFFER_PATTERN_FAILED;
	for (unsigned pass=0;pass<4;pass++) {
		for (unsigned w=0;w<TT_OUTPUT_BUFFER_WORDS;w++) io.OutputBufferWrite(w,PioneerHostPagePattern(pass,w));
		for (unsigned w=0;w<TT_OUTPUT_BUFFER_WORDS;w++) out.mismatches+=io.OutputBufferRead(w)!=PioneerHostPagePattern(pass,w);
		if (out.mismatches) return false;
		out.patterns++;
	}
	if (!PioneerOutputBufferMapStable(io,out,input)) { out.state=TT_OUTPUT_BUFFER_MAP_CHANGED; return false; }
	out.ready=true; out.state=TT_OUTPUT_BUFFER_READY; return true;
}
template<class IO>
void PioneerQuarantineOutputBuffer(IO& io,PioneerOutputBufferLease& out)
{
	if (out.allocated && !out.released) { out.ready=false; out.quarantined=true; io.QuarantineOutputBuffer(); }
}
template<class IO>
bool PioneerReleaseOutputBuffer(IO& io,PioneerOutputBufferLease& out,bool controllerKnownSafe)
{
	if (!out.allocated || out.released) return false;
	if (!controllerKnownSafe || out.pending || out.quarantined) { PioneerQuarantineOutputBuffer(io,out); return false; }
	if (io.ReleaseOutputBuffer()!=0) { out.state=TT_OUTPUT_BUFFER_RELEASE_FAILED; PioneerQuarantineOutputBuffer(io,out); return false; }
	out.ready=false; out.released=true; return true;
}
#endif
