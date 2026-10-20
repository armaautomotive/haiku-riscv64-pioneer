/* SPDX-License-Identifier: MIT */
#define PIONEER_WIDE_OWNER_MODEL_ONLY
#include "pioneer_wide_owner_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38OutputDmaOwner.h"
struct OwnerIO:WideOwnerIO {
	PioneerWideOutputLease* owner=nullptr;
	unsigned pci[2]={0x402,0x402},writes=0,reads=0,failWrite=0,failRead=0;
	int HostPageMap(uint64_t& at,uint64_t& bytes) { auto h=Input(); at=h.physical; bytes=h.bytes; return 0; }
	int OutputBufferMap(uint64_t& at,uint64_t& bytes) { at=Bulk().physical; bytes=16384; return 0; }
	int OutputBufferPageMap(unsigned page,uint64_t& at,uint64_t& bytes) { at=Bulk().physical+page*4096; bytes=4096; return 0; }
	void HostPciWrite(unsigned peer,unsigned at,unsigned bytes,unsigned value) {
		assert(owner && owner->pending && peer<2 && at==4 && bytes==2);
		if (++writes!=failWrite) pci[peer]=value;
	}
	unsigned HostPciRead(unsigned peer,unsigned at,unsigned bytes) {
		assert(peer<2 && at==4 && bytes==2); return pci[peer]^(++reads==failRead?4:0);
	}
};
int main()
{
	{
		OwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease wide; io.owner=&wide;
		assert(PioneerPrepareWideOutput(io,input,bulk,wide) && PioneerQ38OutputDmaOwner(io,input,bulk,wide,false));
		assert(PioneerQ38OutputDmaEnable(io,wide) && wide.pending && io.writes==2 && io.reads==2);
		assert(PioneerQ38OutputDmaOwner(io,input,bulk,wide,true) && !PioneerQ38OutputDmaOwner(io,input,bulk,wide,false));
		assert(PioneerQ38OutputDmaDisableKnownComplete(io) && wide.pending && io.writes==4 && io.reads==4);
		wide.pending=false; // future controller ONLY after completed DMA+BME-off+held/source proof
		assert(PioneerQ38OutputDmaOwner(io,input,bulk,wide,false));
	}
	for (unsigned step=1;step<=4;step++) for (bool readFault:{false,true}) {
		OwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease wide; io.owner=&wide;
		assert(PioneerPrepareWideOutput(io,input,bulk,wide));
		if (readFault) io.failRead=step; else io.failWrite=step;
		bool enabled=PioneerQ38OutputDmaEnable(io,wide);
		if (step<=2) assert(!enabled && io.writes==step && io.reads==step);
		else assert(enabled && !PioneerQ38OutputDmaDisableKnownComplete(io) && io.writes==step && io.reads==step);
		assert(wide.pending); unsigned writes=io.writes,reads=io.reads;
		PioneerQuarantineWideOutput(io,wide);
		assert(wide.quarantined && !wide.ready && !PioneerReleaseWideOutput(io,wide,true));
		assert(!PioneerQ38OutputDmaEnable(io,wide) && io.writes==writes && io.reads==reads && !io.released);
	}
	for (unsigned page=0;page<256;page++) {
		OwnerIO io; auto input=Input(); auto bulk=Bulk(); PioneerWideOutputLease wide; io.owner=&wide;
		assert(PioneerPrepareWideOutput(io,input,bulk,wide)); io.badPage=int(page);
		assert(!PioneerQ38OutputDmaOwner(io,input,bulk,wide,false) && !io.writes && !io.reads);
	}
	puts("PASS OFFLINE output DMA owner: pending BEFORE every BME write, all4 enable/disable write/read failures stop once, no disable/reset/retry cleanup, all256 physical pages, quarantine forbids release/re-arm;NOT native integration");
}
