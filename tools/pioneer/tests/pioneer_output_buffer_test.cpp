/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerOutputBuffer.h"
struct OutputBufferMock {
	uint64_t physical=UINT64_C(0x180000000),bytes=16384;
	uint32_t data[4096]={};
	unsigned writes=0,drop=0,maps=0,pageMaps=0,releases=0,releaseAttempts=0,allocations=0;
	unsigned badPage=~0U,shortPage=~0U;
	bool allocated=false,cached=true,quarantined=false,allocationFails=false,cacheFails=false,releaseFails=false,mapFails=false,lateMap=false;
	int AllocateOutputBuffer() { allocations++; assert(!allocated); if (allocationFails) return -1; allocated=true; return 0; }
	int OutputBufferMap(uint64_t& p,uint64_t& b) { maps++; p=physical+(lateMap && maps>2?4096:0); b=bytes; return mapFails?-1:0; }
	int OutputBufferPageMap(unsigned page,uint64_t& p,uint64_t& b) {
		assert(page<4 && allocated); pageMaps++; p=physical+page*4096+(page==badPage?4096:0); b=page==shortPage?4095:4096; return 0;
	}
	int SetOutputBufferNonCached() { assert(allocated && !writes); if (cacheFails) return -1; cached=false; return 0; }
	void OutputBufferWrite(unsigned w,uint32_t value) { assert(allocated && !cached && !quarantined && w<4096); if (++writes!=drop) data[w]=value; }
	uint32_t OutputBufferRead(unsigned w) { assert(allocated && !cached && w<4096); return data[w]; }
	int ReleaseOutputBuffer() { assert(allocated && !quarantined); releaseAttempts++; if (releaseFails) return -1; releases++; allocated=false; return 0; }
	void QuarantineOutputBuffer() { assert(allocated && !releases); quarantined=true; }
};
static tt_host_page_result Input()
{
	tt_host_page_result r={}; r.version=1; r.state=TT_HOST_PAGE_READY; r.allocated=r.memory_type_set=r.map_stable=1;
	r.physical=UINT64_C(0x120000000); r.bytes=4096; return r;
}
int main()
{
	auto input=Input(); unsigned stores=0;
	{
		OutputBufferMock io; PioneerOutputBufferLease out;
		assert(PioneerPrepareOutputBuffer(io,input,out) && out.ready && out.patterns==4 && io.pageMaps==8);
		stores=io.writes; assert(stores==16384 && !PioneerPrepareOutputBuffer(io,input,out) && io.allocations==1);
		assert(PioneerReleaseOutputBuffer(io,out,true) && out.released && io.releases==1);
		assert(!PioneerReleaseOutputBuffer(io,out,true) && !PioneerPrepareOutputBuffer(io,input,out));
	}
	for (unsigned drop=1;drop<=stores;drop++) {
		OutputBufferMock io; PioneerOutputBufferLease out; io.drop=drop;
		assert(!PioneerPrepareOutputBuffer(io,input,out) && out.mismatches && !out.ready && !io.quarantined);
		assert(PioneerReleaseOutputBuffer(io,out,true) && io.releases==1); // NO DMA ever existed
	}
	for (unsigned page=0;page<4;page++) for (unsigned fault=0;fault<2;fault++) {
		OutputBufferMock io; PioneerOutputBufferLease out;
		if (fault) io.shortPage=page; else io.badPage=page;
		assert(!PioneerPrepareOutputBuffer(io,input,out) && !io.writes && out.state==TT_OUTPUT_BUFFER_BAD_MAP);
		assert(PioneerReleaseOutputBuffer(io,out,true));
	}
	for (uint64_t bad:{UINT64_C(0),UINT64_C(0xfffff000),UINT64_C(0x180000001),UINT64_C(0xfffffffff000),UINT64_C(0x1000000000000)}) {
		OutputBufferMock io; PioneerOutputBufferLease out; io.physical=bad;
		assert(!PioneerPrepareOutputBuffer(io,input,out) && !io.writes && !out.ready);
		assert(PioneerReleaseOutputBuffer(io,out,true));
	}
	for (unsigned page=0;page<4;page++) {
		OutputBufferMock io; PioneerOutputBufferLease out; auto alias=input; alias.physical=io.physical+page*4096;
		assert(!PioneerPrepareOutputBuffer(io,alias,out) && !io.writes && PioneerReleaseOutputBuffer(io,out,true));
	}
	assert(PioneerOutputBufferAddress(UINT64_C(0xffffffffc000),16384));
	assert(!PioneerOutputBufferAddress(UINT64_C(0x180000000),4096));
	for (unsigned fault=0;fault<5;fault++) {
		OutputBufferMock io; PioneerOutputBufferLease out;
		io.allocationFails=fault==0; io.cacheFails=fault==1; io.mapFails=fault==2; io.lateMap=fault==3; if (fault==4) io.bytes=32768;
		assert(!PioneerPrepareOutputBuffer(io,input,out) && !out.ready);
		if (out.allocated) assert(PioneerReleaseOutputBuffer(io,out,true)); else assert(!io.releases);
	}
	for (unsigned fault=0;fault<3;fault++) {
		OutputBufferMock io; PioneerOutputBufferLease out; assert(PioneerPrepareOutputBuffer(io,input,out));
		out.pending=fault==0; if (fault==1) PioneerQuarantineOutputBuffer(io,out);
		assert(!PioneerReleaseOutputBuffer(io,out,fault!=2) && io.quarantined && out.quarantined && !io.releases);
		assert(!PioneerReleaseOutputBuffer(io,out,true) && !io.releases);
	}
	{
		OutputBufferMock io; PioneerOutputBufferLease out; assert(PioneerPrepareOutputBuffer(io,input,out));
		io.releaseFails=true;
		assert(!PioneerReleaseOutputBuffer(io,out,true) && io.quarantined && !io.releases && io.releaseAttempts==1 && io.allocated);
		assert(!PioneerReleaseOutputBuffer(io,out,true) && io.releaseAttempts==1);
	}
	puts("PASS: OFFLINE private16KiB output ownership, all4 physical pages, no input alias, every16384 lost store, cache/map failures, no release after pending/uncertainty; NO native/DMA interface");
}
