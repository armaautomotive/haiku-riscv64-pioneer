/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryDbi.h"
using uint8=uint8_t;using uint32=uint32_t;
enum { TT_PIONEER_RANGE_COUNT=1 };
struct tt_pioneer_profile {
	uint64_t bar0_host_start=1,bar0_host_end=2,bar2_host_start=3,bar2_host_end=4;
	int ranges[1]={};
};
struct PioneerBlackholeBarPlan {
	uint64_t hostStart[2]={1,3},hostEnd[2]={2,4};uint32 bars[6]={},bridge[3]={};
};
struct PioneerPciePayloadSnapshot {
	uint32 capability=0x70,version=2,supported=2,control=0x2040;
};
struct FakeIO {
	uint32 page[1024]={},tlb[3]={5,6,7};
	const volatile uint32* sii=page+0x134/4;
	bool route=true,pci=true,changeOnRead=false;
	unsigned writes=0,dropWrite=0;
	uint32 ReadNodeId(){return 2;}
	bool Quiet(){return route;}
	uint32 CardRead(unsigned,unsigned){return 0;}
	uint32 RootRead(unsigned offset,unsigned){
		if(offset==0)return 0x20421e30;if(offset==8)return 0x06040000;
		if(offset==0x18)return 0x003f0100;if(offset==0x20)return 0x0000fff0;return 0;
	}
	uint32 DramTlbRead(unsigned s,unsigned n){assert(s==0 && n<3);return tlb[n];}
	void DramTlbWrite(unsigned s,unsigned n,uint32 value){
		assert(s==0 && n<3);if (++writes!=dropWrite) tlb[n]=value;
	}
};
static FakeIO* current=nullptr;
static uint32 PioneerRead32(const volatile uint32* address){
	if (current->changeOnRead) current->route=false;
	return *address;
}
static bool PioneerPciePayloadRead(FakeIO& io,PioneerPciePayloadSnapshot& out){out={};return io.pci;}
static bool PioneerMinimumBarLayoutMatches(const tt_pioneer_profile&){return true;}
static bool PioneerPlanBlackholeBars(const int*,unsigned,PioneerBlackholeBarPlan& p){p={};return true;}
static bool BlackholeCardControlsQuiet(FakeIO& io){return io.route;}
static bool BlackholeHostSourceRoute(const uint32*){return true;}
struct FakeDram {};
static bool BlackholeDramSourceStable(FakeIO& io,const FakeDram&){return io.route;}
static bool BlackholeHostPcieStatus(FakeIO& io,unsigned,uint32& status){status=0;return io.route;}
struct PioneerStoreContext {
	tt_pioneer_profile profile;FakeIO io;PioneerRecoveryDbiSnapshot recoveryDbi;
	struct { struct { struct {
		struct { uint32 source[1][16]={}; } noc;
		struct { FakeDram dram; } baseline;
	} baseline; } baseline; } baseline;
};
#include "pioneer_dbi_capture.inc"
int main(){
	bool unsafe=false;PioneerStoreContext good;current=&good.io;good.io.page[0x78/4]=0x00502042;
	assert(CaptureQwen35RecoveryDbi(good,unsafe) && !unsafe && good.recoveryDbi.valid
		&& good.recoveryDbi.mps==0x40 && good.io.tlb[0]==5 && good.io.tlb[1]==6 && good.io.tlb[2]==7);
	PioneerStoreContext rejected;current=&rejected.io;rejected.io.pci=false;
	assert(!CaptureQwen35RecoveryDbi(rejected,unsafe) && unsafe && !rejected.io.writes);
	PioneerStoreContext mapFailure;current=&mapFailure.io;mapFailure.io.dropWrite=1;
	assert(!CaptureQwen35RecoveryDbi(mapFailure,unsafe) && !unsafe && !mapFailure.recoveryDbi.valid
		&& mapFailure.io.tlb[0]==5 && mapFailure.io.tlb[1]==6 && mapFailure.io.tlb[2]==7);
	PioneerStoreContext restoreFailure;current=&restoreFailure.io;restoreFailure.io.page[0x78/4]=0x00502042;
	restoreFailure.io.dropWrite=4;
	assert(!CaptureQwen35RecoveryDbi(restoreFailure,unsafe) && unsafe && !restoreFailure.recoveryDbi.valid);
	PioneerStoreContext drift;current=&drift.io;drift.io.page[0x78/4]=0x00502042;drift.io.changeOnRead=true;
	assert(!CaptureQwen35RecoveryDbi(drift,unsafe) && unsafe && drift.io.writes==3 && !drift.recoveryDbi.valid);
	puts("PASS actual healthy-session DBI capture adapter: mapped, refused, dropped writes, route drift; SIMULATED ONLY");
}
