/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
struct IO {
	uint32_t card[64]={},root[64]={}; unsigned writes=0,unmaps=0,fail=0,reads=0;
	bool unmapFailure=false,failedRead=false;
	IO() {
		card[1]=root[1]=0x402;
		const unsigned bars[]={0x0c,0x49,0x2000000c,0x49,0x0c,0xfffffff8};
		for (unsigned i=0;i<6;i++) card[4+i]=bars[i];
		root[9]=0x20010001; root[10]=root[11]=0x49;
	}
	bool Unmap() { assert(!failedRead); unmaps++; return !unmapFailure; }
	void Write(uint32_t* base,unsigned a,unsigned size,uint32_t value) {
		assert(!failedRead && (size==2 || size==4) && !(a&3));
		writes++; base[a/4]=writes==fail?value^1:value;
	}
	void CardWrite(unsigned a,unsigned size,uint32_t value) { Write(card,a,size,value); }
	void RootWrite(unsigned a,unsigned size,uint32_t value) { Write(root,a,size,value); }
	uint32_t Read(uint32_t* base,unsigned a,unsigned) { reads++; if (writes==fail && fail) failedRead=true; return base[a/4]; }
	uint32_t CardRead(unsigned a,unsigned size) { return Read(card,a,size); }
	uint32_t RootRead(unsigned a,unsigned size) { return Read(root,a,size); }
};
static tt_pioneer_mmio_result Result()
{
	tt_pioneer_mmio_result r={}; r.version=1; r.state=TT_PIONEER_MMIO_READ_OK;
	for (unsigned i=0;i<6;i++) r.bars_before[i]=i%2?0:0x0c;
	r.bridge_before[0]=0x1fff1; return r;
}
int main()
{
	{
		IO io; auto r=Result(); tt_pioneer_mmio_transaction t={true,true};
		assert(PioneerQ38DispatchMmioEnd(io,r,t) && r.restored==1 && r.state==TT_PIONEER_MMIO_READ_OK && io.writes==11 && io.unmaps==1);
		unsigned writes=io.writes,reads=io.reads;
		assert(PioneerQ38DispatchMmioEnd(io,r,t) && io.writes==writes && io.reads==reads && io.unmaps==1);
	}
	for (unsigned fault=0;fault<=11;fault++) {
		IO io; auto r=Result(); tt_pioneer_mmio_transaction t={true,true};
		io.unmapFailure=!fault; io.fail=fault;
		assert(!PioneerQ38DispatchMmioEnd(io,r,t) && !r.restored && r.state==TT_PIONEER_MMIO_RESTORE_FAILED && !t.config_attempted);
		assert(io.writes==fault && io.unmaps==1);
		unsigned writes=io.writes,reads=io.reads;
		assert(!PioneerQ38DispatchMmioEnd(io,r,t) && io.writes==writes && io.reads==reads && io.unmaps==1);
	}
	puts("PASS OFFLINE54 strict final MMIO transaction:11 exact decoder/BAR/bridge writes/readbacks,unmap+EVERY write/readback fault stops ALL later writes,one-shot NO retry or repeated IO;called only AFTER all physical terminal/held/source/restoration proofs;NO native qualification");
}
