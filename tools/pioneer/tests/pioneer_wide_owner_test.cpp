/* SPDX-License-Identifier: MIT */
#define PIONEER_OUTPUT_BUFFER_MOCK_ONLY
#include "pioneer_output_buffer_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerWideOutput.h"
#include <vector>
struct WideOwnerIO {
	std::vector<uint32_t> data=std::vector<uint32_t>(TT_WIDE_OUTPUT_WORDS,0xabcdef01);
	uint64_t physical=UINT64_C(0x140000000),bytes=TT_WIDE_OUTPUT_BYTES;
	unsigned allocated=0,released=0,stores=0,loads=0,dropStore=0,corruptRead=0;
	int badPage=-1;
	bool cached=true,dirty=false,quarantined=false,failAllocate=false,failCache=false,lateMap=false,failRelease=false;
	int AllocateWideOutput() { assert(!allocated); if (failAllocate) return -1; allocated++; return 0; }
	int WideOutputMap(uint64_t& at,uint64_t& length) { assert(allocated && !released); at=physical+(lateMap && stores?TT_WIDE_OUTPUT_BYTES:0); length=bytes; return 0; }
	int WideOutputPageMap(unsigned page,uint64_t& at,uint64_t& length) { assert(page<256 && allocated && !released); at=physical+page*4096+(int(page)==badPage?4096:0); length=4096; return 0; }
	int SetWideOutputNonCached() { assert(allocated && !stores); if (failCache) return -1; cached=false; return 0; }
	void WideHostFence() { dirty=false; }
	void WideHostStore(unsigned i,uint32_t value) { assert(!cached && !quarantined && i<data.size()); dirty=true; if (++stores!=dropStore) data[i]=value; }
	uint32_t WideHostLoad(unsigned i) { assert(!cached && !quarantined && !dirty && i<data.size()); return data[i]^(++loads==corruptRead?1:0); }
	int ReleaseWideOutput() { assert(allocated && !released && !quarantined); if (failRelease) return -1; released++; return 0; }
	void QuarantineWideOutput() { quarantined=true; }
};
static PioneerOutputBufferLease Bulk() {
	PioneerOutputBufferLease b; b.allocated=b.ready=b.memory_type_set=true; b.physical=UINT64_C(0x100100000); return b;
}
int main()
{
	{
		WideOwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease out;
		assert(PioneerPrepareWideOutput(io,input,bulk,out) && out.ready && out.patterns==4 && !out.mismatches
			&& io.stores==4*TT_WIDE_OUTPUT_WORDS && io.loads==io.stores);
		assert(PioneerWideOutputMapStable(io,out,input,bulk));
		assert(PioneerReleaseWideOutput(io,out,true) && out.released && io.released==1);
	}
	for (unsigned page=0;page<256;page++) {
		WideOwnerIO io; io.badPage=int(page); auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease out;
		assert(!PioneerPrepareWideOutput(io,input,bulk,out) && out.allocated && !io.stores && !out.ready);
		assert(PioneerReleaseWideOutput(io,out,true)); // CPU-only preparation: no DMA was ever possible
	}
	for (unsigned fault=0;fault<11;fault++) {
		WideOwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease out;
		if (fault==0) io.physical+=4096; if (fault==1) io.bytes=16384; if (fault==2) io.physical=UINT64_C(0x1000000000000);
		if (fault==3) io.physical=UINT64_C(0xfff00000); if (fault==4) input.physical=io.physical+4096;
		if (fault==5) bulk.physical=io.physical+4096; if (fault==6) io.failCache=true;
		if (fault==7) io.dropStore=1; if (fault==8) io.corruptRead=TT_WIDE_OUTPUT_WORDS;
		if (fault==9) io.lateMap=true; if (fault==10) io.failAllocate=true;
		assert(!PioneerPrepareWideOutput(io,input,bulk,out) && !out.ready);
		if (out.allocated) assert(PioneerReleaseWideOutput(io,out,true));
	}
	for (unsigned fault=0;fault<7;fault++) {
		WideOwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease out;
		if (fault==0) input.released=true; if (fault==1) bulk.ready=false; if (fault==2) bulk.quarantined=true;
		if (fault==3) bulk.pending=true; if (fault==4) bulk.memory_type_set=false; if (fault==5) bulk.released=true;
		if (fault==6) bulk.allocated=false;
		assert(!PioneerPrepareWideOutput(io,input,bulk,out) && !io.allocated);
	}
	for (unsigned fault=0;fault<4;fault++) {
		WideOwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease out;
		assert(PioneerPrepareWideOutput(io,input,bulk,out));
		if (fault==0) out.pending=true; if (fault==1) io.failRelease=true; if (fault==2) out.quarantined=true;
		assert(!PioneerReleaseWideOutput(io,out,fault!=3) && io.quarantined && out.quarantined && !io.released);
	}
	assert(PioneerWideOutputAddress(UINT64_C(0xfffffff00000),0x100000));
	puts("PASS OFFLINE independent1MiB wide owner,256 page mappings,48-bit/alignment/three-owner separation,all-word patterns,changed maps,retained quarantine; NO device/DMA/driver exposure");
}
