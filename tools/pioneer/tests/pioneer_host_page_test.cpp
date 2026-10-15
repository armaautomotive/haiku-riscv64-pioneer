/* SPDX-License-Identifier: MIT */
#include <cassert>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerHostPage.h"
struct PageMock {
	uint32_t page[TT_HOST_PAGE_WORDS]={};
	uint64_t physical=UINT64_C(0x123456000),bytes=4096;
	unsigned writes=0,drop=0,maps=0,releases=0;
	bool allocated=false,cached=true,allocFail=false,mapFail=false,cacheFail=false,mapChanged=false,releaseFail=false;
	int32_t AllocateHostPage() { assert(!allocated); allocated=!allocFail; return allocFail?-1:0; }
	int32_t HostPageMap(uint64_t& address,uint64_t& length) {
		assert(allocated); maps++; address=physical+(mapChanged && maps>1?4096:0); length=bytes; return mapFail?-2:0;
	}
	int32_t SetHostPageNonCached() { assert(allocated); cached=cacheFail; return cacheFail?-3:0; }
	void HostPageWrite(unsigned w,uint32_t value) { assert(allocated && !cached && w<TT_HOST_PAGE_WORDS); if (++writes!=drop) page[w]=value; }
	uint32_t HostPageRead(unsigned w) { assert(allocated && !cached && w<TT_HOST_PAGE_WORDS); return page[w]; }
	int32_t ReleaseHostPage() { assert(allocated && releases==0); releases++; if (!releaseFail) allocated=false; return releaseFail?-4:0; }
};
int main()
{
	PageMock io; tt_host_page_result r;
	assert(PioneerHostPageProbe(io,r));
	assert(r.version==1 && r.state==TT_HOST_PAGE_READY && r.memory_type_set && r.patterns==2
		&& r.map_stable && r.released && r.address_bits==33 && io.releases==1 && !io.allocated);
	for (unsigned drop=1;drop<=2048;drop++) {
		io={}; io.drop=drop; assert(!PioneerHostPageProbe(io,r) && r.state==TT_HOST_PAGE_PATTERN_FAILED && r.released);
	}
	for (unsigned fault=0;fault<6;fault++) {
		io={}; io.allocFail=fault==0; io.mapFail=fault==1; io.cacheFail=fault==2;
		io.mapChanged=fault==3; io.releaseFail=fault==4; if (fault==5) io.physical=UINT64_C(0xfffff000);
		assert(!PioneerHostPageProbe(io,r));
		assert(io.releases==(fault==0?0U:1U));
		if (fault!=4) assert(r.released);
		if (fault==1 || fault==2 || fault==5) assert(io.writes==0);
	}
	assert(!PioneerHostPageAddress(UINT64_MAX-4095,4096));
	assert(!PioneerHostPageAddress(UINT64_C(0x100001000),4095));
	assert(!PioneerHostPageAddress(UINT64_C(0x100001001),4096));
	assert(!PioneerHostPageAddress(UINT64_C(0x1000000000000),4096));
	assert(PioneerHostPageAddress(UINT64_C(0xfffffffff000),4096));
	io={}; assert(PioneerPrepareHostPage(io,r) && io.allocated && io.releases==0 && !r.released);
	assert(io.ReleaseHostPage()==0);
}
