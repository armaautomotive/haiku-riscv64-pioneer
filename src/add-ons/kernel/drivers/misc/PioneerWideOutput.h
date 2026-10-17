/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_WIDE_OUTPUT_H
#define PIONEER_WIDE_OUTPUT_H
#include "PioneerOutputBuffer.h"
// Contract10 private wide-output owner. No caller supplies its physical address.
// Independent third owner.1MiB physical alignment also prevents any single
// NoC subspan crossing a4GiB address boundary; no unqualified carry behavior.
enum { TT_WIDE_OUTPUT_BYTES=0x100000,TT_WIDE_OUTPUT_PAGES=256,TT_WIDE_OUTPUT_WORDS=262144 };
struct PioneerWideOutputLease: PioneerOutputBufferLease {};
static inline bool PioneerWideOutputAddress(uint64_t physical,uint64_t bytes)
{
	return bytes==TT_WIDE_OUTPUT_BYTES && physical>=UINT64_C(0x100000000) && !(physical&(TT_WIDE_OUTPUT_BYTES-1))
		&& physical<UINT64_C(0x1000000000000) && bytes<=UINT64_C(0x1000000000000)-physical;
}
static inline bool PioneerWideOutputDistinct(uint64_t physical,const tt_host_page_result& input,const PioneerOutputBufferLease& bulk)
{
	return PioneerWideOutputAddress(physical,TT_WIDE_OUTPUT_BYTES) && PioneerOutputBufferDistinct(bulk.physical,input)
		&& bulk.allocated && bulk.ready && bulk.memory_type_set && !bulk.released && !bulk.quarantined && !bulk.pending
		&& (physical+TT_WIDE_OUTPUT_BYTES<=input.physical || input.physical+input.bytes<=physical)
		&& (physical+TT_WIDE_OUTPUT_BYTES<=bulk.physical || bulk.physical+TT_OUTPUT_BUFFER_BYTES<=physical);
}
template<class IO>
bool PioneerWideOutputMapStable(IO& io,const PioneerWideOutputLease& out,const tt_host_page_result& input,const PioneerOutputBufferLease& bulk)
{
	uint64_t physical=0,bytes=0;
	if (!out.allocated || out.released || out.quarantined || io.WideOutputMap(physical,bytes)!=0 || physical!=out.physical
		|| !PioneerWideOutputAddress(physical,bytes) || !PioneerWideOutputDistinct(physical,input,bulk)) return false;
	for (unsigned page=0;page<TT_WIDE_OUTPUT_PAGES;page++) {
		uint64_t at=0,length=0;
		if (io.WideOutputPageMap(page,at,length)!=0 || length<4096 || at!=physical+page*4096) return false;
	}
	return true;
}
template<class IO>
bool PioneerPrepareWideOutput(IO& io,const tt_host_page_result& input,const PioneerOutputBufferLease& bulk,PioneerWideOutputLease& out)
{
	if (out.allocated || out.released || out.quarantined || out.pending || !bulk.allocated || !bulk.ready || !bulk.memory_type_set || bulk.released || bulk.quarantined || bulk.pending
		|| !PioneerOutputBufferDistinct(bulk.physical,input)) return false;
	if (io.AllocateWideOutput()!=0) { out.state=TT_OUTPUT_BUFFER_ALLOC_FAILED; return false; }
	out.allocated=true; out.state=TT_OUTPUT_BUFFER_BAD_MAP;
	uint64_t at=0,bytes=0;
	if (io.WideOutputMap(at,bytes)!=0 || !PioneerWideOutputAddress(at,bytes) || !PioneerWideOutputDistinct(at,input,bulk)) return false;
	out.physical=at;
	if (!PioneerWideOutputMapStable(io,out,input,bulk)) return false;
	out.state=TT_OUTPUT_BUFFER_CACHE_FAILED;
	if (io.SetWideOutputNonCached()!=0) return false;
	out.memory_type_set=true; out.state=TT_OUTPUT_BUFFER_PATTERN_FAILED;
	for (unsigned pass=0;pass<4;pass++) {
		io.WideHostFence();
		for (unsigned i=0;i<TT_WIDE_OUTPUT_WORDS;i++) io.WideHostStore(i,PioneerHostPagePattern(pass,i));
		io.WideHostFence();
		for (unsigned i=0;i<TT_WIDE_OUTPUT_WORDS;i++) out.mismatches+=io.WideHostLoad(i)!=PioneerHostPagePattern(pass,i);
		io.WideHostFence();
		if (out.mismatches) return false;
		out.patterns++;
	}
	if (!PioneerWideOutputMapStable(io,out,input,bulk)) { out.state=TT_OUTPUT_BUFFER_MAP_CHANGED; return false; }
	out.ready=true; out.state=TT_OUTPUT_BUFFER_READY; return true;
}
template<class IO> auto PioneerWideQuarantineIO(IO& io,int)->decltype(io.QuarantineWideOutput(),void()) { io.QuarantineWideOutput(); }
template<class IO> void PioneerWideQuarantineIO(IO& io,long) { io.QuarantineHostPage(); }
template<class IO> void PioneerQuarantineWideOutput(IO& io,PioneerWideOutputLease& out)
{
	if (out.allocated && !out.released) { out.ready=false; out.quarantined=true; PioneerWideQuarantineIO(io,0); }
}
template<class IO> bool PioneerReleaseWideOutput(IO& io,PioneerWideOutputLease& out,bool knownSafe)
{
	if (!out.allocated || out.released) return false;
	if (!knownSafe || out.pending || out.quarantined) { PioneerQuarantineWideOutput(io,out); return false; }
	if (io.ReleaseWideOutput()!=0) { out.state=TT_OUTPUT_BUFFER_RELEASE_FAILED; PioneerQuarantineWideOutput(io,out); return false; }
	out.ready=false; out.released=true; return true;
}
static inline bool PioneerWideOutputSummary(const uint32_t* s,unsigned n,unsigned workers,unsigned nonce,const uint32_t* before,const uint32_t* after,bool trusted=false)
{
	if (!s || !workers || workers>119 || !n || n>TT_WIDE_OUTPUT_WORDS || (n&31) || !nonce || nonce>=0x7fffffff
		|| s[4]!=workers || s[5]!=n || s[6]!=nonce || s[7]!=6) return false;
	unsigned r=s[0],minimum=(trusted?5:12)*workers,w=3*workers,wf=2*workers+n/16;
	if (r<minimum || r>minimum+2*65535*workers || ((r-minimum)&1) || s[1]!=r-2*workers+n/8
		|| s[2]!=w || s[3]!=wf || before[2]!=before[6] || after[2]!=after[6]) return false;
	const unsigned delta[]={r,s[1],r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
// Old-mode test adapters need not expose any third-owner authority.
template<class IO> auto PioneerWideMapping(IO& io,const PioneerWideOutputLease& out,const tt_host_page_result& input,const PioneerOutputBufferLease& bulk,int)
	->decltype(io.WideHostLoad(0U),bool()) { return PioneerWideOutputMapStable(io,out,input,bulk); }
template<class IO> bool PioneerWideMapping(IO&,const PioneerWideOutputLease&,const tt_host_page_result&,const PioneerOutputBufferLease&,long) { return false; }
template<class IO> auto PioneerWideLoad(IO& io,unsigned word,int)->decltype(io.WideHostLoad(word)) { return io.WideHostLoad(word); }
template<class IO> uint32_t PioneerWideLoad(IO&,unsigned,long) { return UINT32_MAX; }
template<class IO> auto PioneerWideStore(IO& io,unsigned word,uint32_t value,int)->decltype(io.WideHostStore(word,value),bool()) { io.WideHostStore(word,value); return true; }
template<class IO> bool PioneerWideStore(IO&,unsigned,uint32_t,long) { return false; }
#endif
