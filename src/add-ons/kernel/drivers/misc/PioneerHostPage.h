/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_HOST_PAGE_H
#define PIONEER_HOST_PAGE_H
#include <stdint.h>
enum {
	TT_HOST_PAGE_REFUSED=0,TT_HOST_PAGE_READY=1,TT_HOST_PAGE_ALLOC_FAILED=2,
	TT_HOST_PAGE_BAD_MAP=3,TT_HOST_PAGE_CACHE_FAILED=4,TT_HOST_PAGE_PATTERN_FAILED=5,
	TT_HOST_PAGE_MAP_CHANGED=6,TT_HOST_PAGE_RELEASE_FAILED=7,
	TT_HOST_PAGE_BYTES=4096,TT_HOST_PAGE_WORDS=1024
};
struct tt_host_page_result {
	uint32_t version,state,allocated,memory_type_set,patterns,map_stable,released,mismatches;
	uint32_t bytes,address_bits;
	int32_t allocation_status,map_status,cache_status,release_status;
	uint64_t physical;
};
static inline bool PioneerHostPageAddress(uint64_t physical,uint64_t bytes)
{
	return bytes==TT_HOST_PAGE_BYTES && physical>=UINT64_C(0x100000000)
		&& (physical&(TT_HOST_PAGE_BYTES-1))==0
		&& physical<UINT64_C(0x1000000000000)
		&& bytes<=UINT64_C(0x1000000000000)-physical;
}
static inline uint32_t PioneerHostPagePattern(unsigned pass,unsigned word)
{
	uint32_t value=0x5a69c3e7U^(word*0x10203U)^((pass/2)*0x31415927U);
	return (pass&1)?~value:value;
}
// Prepare an owned, wired 4KiB page. No PCI/MMIO/DMA APIs exist in this
// interface. The caller retains ownership even on failure after allocation.
template<class IO>
bool PioneerPrepareHostPage(IO& io,tt_host_page_result& r)
{
	r={}; r.version=1; r.released=1;
	r.allocation_status=io.AllocateHostPage();
	if (r.allocation_status!=0) { r.state=TT_HOST_PAGE_ALLOC_FAILED; return false; }
	r.allocated=1; r.released=0;
	uint64_t physical=0,bytes=0;
	r.map_status=io.HostPageMap(physical,bytes);
	r.state=TT_HOST_PAGE_BAD_MAP;
	if (r.map_status!=0 || !PioneerHostPageAddress(physical,bytes)) return false;
	r.physical=physical; r.bytes=uint32_t(bytes);
	for (uint64_t rest=physical;rest;rest>>=1) r.address_bits++;
	r.cache_status=io.SetHostPageNonCached();
	if (r.cache_status!=0) { r.state=TT_HOST_PAGE_CACHE_FAILED; return false; }
	r.memory_type_set=1;
	for (unsigned pass=0;pass<2;pass++) {
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) io.HostPageWrite(w,PioneerHostPagePattern(pass,w));
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.mismatches+=io.HostPageRead(w)!=PioneerHostPagePattern(pass,w);
		if (r.mismatches) { r.state=TT_HOST_PAGE_PATTERN_FAILED; return false; }
		r.patterns++;
	}
	uint64_t after=0,length=0;
	r.map_status=io.HostPageMap(after,length);
	if (r.map_status!=0 || after!=physical || length!=bytes) { r.state=TT_HOST_PAGE_MAP_CHANGED; return false; }
	r.map_stable=1; r.state=TT_HOST_PAGE_READY; return true;
}
template<class IO>
bool PioneerHostPageProbe(IO& io,tt_host_page_result& r)
{
	bool ready=PioneerPrepareHostPage(io,r);
	if (r.allocated) {
		r.release_status=io.ReleaseHostPage(); r.released=r.release_status==0;
		if (!r.released) { r.state=TT_HOST_PAGE_RELEASE_FAILED; return false; }
	}
	return ready;
}
#endif
