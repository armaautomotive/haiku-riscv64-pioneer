/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using area_id=int;
enum {B_OK=0,B_PAGE_SIZE=4096,B_KERNEL_READ_AREA=1,B_KERNEL_WRITE_AREA=2,
	B_UNCACHED_MEMORY=4,B_ANY_KERNEL_ADDRESS=0,TT_PIONEER_RANGE_COUNT=1,TT_DRAM_SOURCE_WORDS=16};
struct tt_pioneer_profile { uint64_t bar0_host_start=0x100000000ULL,bar0_host_end=0,bar2_host_start=0,bar2_host_end=0; unsigned root_bus=0; int ranges[1]={}; };
struct PioneerBlackholeBarPlan { uint64_t hostStart[2]={0x100000000ULL,0},hostEnd[2]={0,0}; uint32 bars[6]={12,0,12,0,12,0},bridge[3]={1,2,3}; };
struct tt_dram_samples_result { uint32 source[16]={}; };
static bool PioneerMinimumBarLayoutMatches(const tt_pioneer_profile&) { return true; }
static bool PioneerPlanBlackholeBars(const int*,int,PioneerBlackholeBarPlan& p) { p={}; return true; }
static uint32 BlackholeDramSourceOffset(unsigned n) { return 0x100+n*4; }
static bool BlackholeDramBuildRoutes(tt_dram_samples_result&) { return true; }
static unsigned PioneerWorkerX(unsigned n) { const unsigned x[]={1,2,3,4,5,6,7,10,11,12,13,14}; return x[n/10]; }
static unsigned PioneerWorkerY(unsigned n) { return 2+n%10; }
static unsigned BlackholeDramSourceX(const uint32*,unsigned x) { return x; }
static bool BlackholeHostSourceRoute(const uint32*) { return true; }
static uint32 BlackholeHostDbiTlbWord(unsigned w) {
	const uint32 v[]={0x100000,0x1001000,0xa40};return v[w];
}
static uint32 BlackholeHostHdmaOffset(unsigned w) { const uint32 v[]={0,0x80,0x100,0x180};return v[w]; }
static int fakePciPendingPeer=-1;
template<class IO> static bool BlackholeHostPcieStatus(IO&,unsigned peer,uint32& s) {
	s=int(peer)==fakePciPendingPeer?0x20:0;return !s;
}
static uint32 BlackholeDramFixedTlbWord(uint32 base,uint32 x,uint32 y,bool readOnly,unsigned w) {
	return w==0?base>>21:w==1?(x<<11)|(y<<17)|(x<<23)|((y&7)<<29):(y>>3)|(readOnly?0xa40:0x40);
}
static uint32 tlbPage[1024],nodePage[1024],resetPage[1024],nocPage[0x12000/4],hdmaPage[1024];
static unsigned mapCalls=0,unmapCalls=0;static int failMap=-1,failUnmap=-1;static bool wrongNode=false;
static uint32 cardBars[6]={12,0,12,0,12,0};
struct Pci {
	uint32 read_pci_config(unsigned,unsigned,unsigned,unsigned offset,unsigned) {
		if(offset==0)return 0x20421e30;if(offset==8)return 0x06040000;if(offset==4)return 0x402;
		if(offset==0x18)return 0x003f0100;if(offset==0x20)return 0x0000fff0;
		if(offset>=0x24 && offset<=0x2c)return (offset-0x24)/4+1;return 0;
	}
} pci,*sPCI=&pci;
struct ConfigIO {
	uint32 Read(uint16 offset,uint8) {
		if(offset==0)return 0xb1401e52;if(offset==0x2c)return 0x00431e52;if(offset==4)return 0x402;
		if(offset>=0x10 && offset<=0x24)return cardBars[(offset-0x10)/4];return 0;
	}
};
static int map_physical_memory(const char*,uint64_t addr,size_t size,int,uint32 flags,void** out) {
	const unsigned call=mapCalls++;if(int(call)==failMap)return -1;
	const uint64_t base=0x100000000ULL;
	if(addr==base+0x1fc00000 && size==4096 && (flags&B_KERNEL_WRITE_AREA)) *out=tlbPage;
	else if(addr==base+0x1fd04000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=nodePage;
	else if(addr==base+0x19312000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=resetPage;
	else if(addr==base+0x19320000 && size==0x12000 && !(flags&B_KERNEL_WRITE_AREA)) *out=nocPage;
	else if(addr==base+0x19180000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=hdmaPage;
	else return -1;
	return int(call);
}
static int delete_area(int a) { unmapCalls++;return a==failUnmap?-1:B_OK; }
static uint32 PioneerRead32(const volatile uint32* p) {
	if(p==nocPage+0x44/4) {
		const uint32 word=tlbPage[0x96c/4+1];const unsigned x=(word>>11)&63,y=(word>>17)&63;
		return (wrongNode?63:x)|(y<<6)|(17U<<12)|(12U<<19);
	}
	if(p==nocPage+0x48/4)return 0x10000;
	return *p;
}
static void PioneerWrite32(volatile uint32* p,uint32 value) {*p=value;}
static unsigned spins=0,mutateAfter=0;
static void spin(unsigned) { if (mutateAfter && ++spins==mutateAfter) hdmaPage[0x80/4]=3; }
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetMapping.inc"

static void ResetFake() {
	memset(tlbPage,0,sizeof(tlbPage));memset(nodePage,0,sizeof(nodePage));
	memset(resetPage,0,sizeof(resetPage));memset(nocPage,0,sizeof(nocPage));memset(hdmaPage,0,sizeof(hdmaPage));
	mapCalls=unmapCalls=0;failMap=failUnmap=-1;wrongNode=false;
	fakePciPendingPeer=-1;spins=mutateAfter=0;
	cardBars[0]=12;nodePage[0x44/4]=2;resetPage[0x1b0/4]=0x47800;
	tlbPage[0x96c/4]=7;tlbPage[0x970/4]=8;tlbPage[0x974/4]=9;
	tlbPage[0x960/4]=10;tlbPage[0x964/4]=11;tlbPage[0x968/4]=12;
}
int main() {
	tt_pioneer_profile profile;PioneerPostResetAuditResult r;
	ResetFake();PioneerPostResetMapping clean(profile);
	assert(clean.Open() && !clean.AuditHostDma() && clean.Audit(r) && r.audited==240
		&& clean.AuditHostDma() && clean.Close());
	assert(unmapCalls==5 && tlbPage[0x96c/4]==7 && tlbPage[0x970/4]==8 && tlbPage[0x974/4]==9
		&& tlbPage[0x960/4]==10 && tlbPage[0x964/4]==11 && tlbPage[0x968/4]==12);
	ResetFake();cardBars[0]=0;PioneerPostResetMapping badBar(profile);
	assert(!badBar.Open() && !mapCalls && badBar.Close());
	ResetFake();failMap=2;PioneerPostResetMapping partial(profile);
	assert(!partial.Open() && partial.Close() && unmapCalls==2);
	ResetFake();wrongNode=true;PioneerPostResetMapping wrong(profile);
	assert(wrong.Open() && !wrong.Audit(r) && r.failure==PioneerPostResetAuditSelect);
	assert(wrong.Close() && tlbPage[0x96c/4]==7);
	ResetFake();nocPage[(0x10000+0x240)/4]=1;PioneerPostResetMapping busy(profile);
	assert(busy.Open() && !busy.Audit(r) && r.failure==PioneerPostResetAuditOutstanding && r.noc==1);
	assert(busy.Close());
	ResetFake();PioneerPostResetMapping retained(profile);
	assert(retained.Open() && retained.SelectPostResetCore(0));
	cardBars[0]=0;assert(!retained.Close() && !unmapCalls);
	cardBars[0]=12;assert(retained.Close());
	ResetFake();PioneerPostResetMapping dmaBusy(profile);
	assert(dmaBusy.Open() && dmaBusy.Audit(r));
	hdmaPage[0x100/4]=1;assert(!dmaBusy.AuditHostDma());
	assert(dmaBusy.Close() && tlbPage[0x960/4]==10);
	ResetFake();PioneerPostResetMapping pending(profile);
	assert(pending.Open() && pending.Audit(r));
	fakePciPendingPeer=0;assert(!pending.AuditHostDma());
	assert(pending.Close());
	ResetFake();PioneerPostResetMapping rootPending(profile);
	assert(rootPending.Open() && rootPending.Audit(r));
	fakePciPendingPeer=1;assert(!rootPending.AuditHostDma());
	assert(rootPending.Close());
	ResetFake();PioneerPostResetMapping changingDma(profile);
	assert(changingDma.Open() && changingDma.Audit(r));
	spins=0;mutateAfter=1;assert(!changingDma.AuditHostDma());
	assert(changingDma.Close());
	ResetFake();nodePage[0x44/4]=11;PioneerPostResetMapping otherPcie(profile);
	assert(otherPcie.Open() && otherPcie.Audit(r) && !otherPcie.AuditHostDma());
	assert(otherPcie.Close());
	puts("PASS post-reset mapping and DMA: core/NoC audit, PCI pending and HDMA busy refusal, both fresh TLB restores; SIMULATED MMIO ONLY");
}
