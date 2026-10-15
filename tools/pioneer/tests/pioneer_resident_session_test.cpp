/* SPDX-License-Identifier: MIT */
#include <cassert>
#define PIONEER_HOST_READ_MOCK_ONLY
#include "pioneer_host_read_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentSession.h"
struct IdleMock : HostReadMock {
	bool outgoing=false,sourceChange=false,restoreTlbFails=false;
	uint32_t ResidentNocRead(uint32_t offset) {
		if (outgoing && offset==0x280) return 1;
		if (sourceChange && offset==BlackholeDramSourceOffset(1)) return source[1]^1;
		return Mock::ResidentNocRead(offset);
	}
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
		if (restoreTlbFails && slot==1 && value==4+word) return;
		Mock::DramTlbWrite(slot,word,value);
	}
};
int main()
{
	assert(PioneerResidentCapacity(4096) && PioneerResidentCapacity(UINT64_C(0x80000000)));
	const uint64_t invalid[]={0,1,4095,4097,UINT64_C(0x80001000),UINT64_MAX};
	for (uint64_t bytes:invalid) assert(!PioneerResidentCapacity(bytes));
	assert(PioneerResidentOffset(8192,0) && PioneerResidentOffset(8192,4096));
	assert(!PioneerResidentOffset(8192,8192) && !PioneerResidentOffset(8192,1));
	assert(!PioneerResidentOffset(8192,UINT64_MAX-4095));
	assert(!PioneerResidentOffset(UINT64_MAX,0));
	assert(PioneerResidentAddress(8192,0)==0x04000000 && PioneerResidentAddress(8192,4096)==0x04001000);
	assert(PioneerResidentAddress(UINT64_C(0x80000000),UINT64_C(0x7ffff000))==0x83fff000);
	assert(PioneerResidentAddress(8192,8192)==UINT32_MAX);
	IdleMock io; auto n=io.Compute(); tt_resident_session_idle_result r;
	assert(PioneerResidentSessionIdle(io,n,r) && r.ready && r.tlb_restored && !r.recovery_required);
	unsigned writes=io.Mock::writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; io.Mock::drop=drop; PioneerResidentSessionIdle(io,n,r);
		assert(r.tlb_restored && !r.recovery_required); io.Mock::Restored();
	}
	for (unsigned fault=0;fault<5;fault++) {
		io={}; io.outgoing=fault==0; io.sourceChange=fault==1; io.restoreTlbFails=fault==2;
		io.irq=fault==3; if (fault==4) io.command[0]=1;
		assert(!PioneerResidentSessionIdle(io,n,r));
		assert((r.recovery_required!=0)==(fault==1 || fault==2));
	}
}
