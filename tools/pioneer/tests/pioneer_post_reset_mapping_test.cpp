/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcTelemetryProbe.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using area_id=int;
enum {B_OK=0,B_NO_MEMORY=-2,B_PAGE_SIZE=4096,B_KERNEL_READ_AREA=1,B_KERNEL_WRITE_AREA=2,
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
static uint32 tlbPage[1024],nodePage[1024],resetPage[1024],nocPage[0x12000/4],hdmaPage[1024],scratchPage[1024],csmPage[0x30000/4],msiPage[1024],dbiPage[1024];
static unsigned mapCalls=0,unmapCalls=0;static int failMap=-1,failUnmap=-1,mapFailureStatus=B_NO_MEMORY;static bool wrongNode=false;
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
static int map_physical_memory(const char*,uint64_t addr,size_t size,int addressSpec,uint32 flags,void** out) {
	assert(addressSpec==(B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY));
	assert((flags&B_KERNEL_READ_AREA) && !(flags&B_UNCACHED_MEMORY));
	const unsigned call=mapCalls++;if(int(call)==failMap)return mapFailureStatus;
	const uint64_t base=0x100000000ULL;
	if(addr==base+0x1fc00000 && size==4096 && (flags&B_KERNEL_WRITE_AREA)) *out=tlbPage;
	else if(addr==base+0x1fd04000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=nodePage;
	else if(addr==base+0x19312000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=resetPage;
	else if(addr==base+0x19320000 && size==0x12000 && !(flags&B_KERNEL_WRITE_AREA)) *out=nocPage;
	else if(addr==base+0x19180000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=hdmaPage;
	else if(addr==base+0x19230000 && size==4096 && !(flags&B_KERNEL_WRITE_AREA)) *out=scratchPage;
	else if(addr==base+0x19200000 && size==0x20000 && !(flags&B_KERNEL_WRITE_AREA)) *out=csmPage;
	else if(addr==base+0x19200000 && size==0x30000 && (flags&B_KERNEL_WRITE_AREA)) *out=csmPage;
	else if(addr==base+0x192b0000 && size==4096 && (flags&B_KERNEL_WRITE_AREA)) *out=msiPage;
	else if(addr==base+0x19000000 && size==4096 && (flags&B_KERNEL_WRITE_AREA)) *out=dbiPage;
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
static bool blockTlbRestore=false,blockDbiTlbRestore=false,dropDbiWrite=false;
static bool fakeQueueTimeout=false,fakeQueueBadReply=false;
static void PioneerWrite32(volatile uint32* p,uint32 value) {
	if (blockTlbRestore && p==tlbPage+0x96c/4 && value==7) return;
	if (blockDbiTlbRestore && p==tlbPage+0x960/4 && value==10) return;
	if (p==dbiPage+0x78/4) {
		assert(!(value&0xffff0000));
		if (!dropDbiWrite) *p=(*p&0xffff0000&~value)|(value&0xffff);
		return;
	}
	*p=value;
	if (p==msiPage && !fakeQueueTimeout) {
		const unsigned base=0x28000/4;
		const unsigned req=csmPage[base],resp=csmPage[base+1];
		assert(csmPage[base+8+((req+7)%8%4)*8]==0xa0);
		csmPage[base+4]=req;csmPage[base+5]=(resp+1)%8;csmPage[base+6]++;
		csmPage[base+40+(resp%4)*8]=fakeQueueBadReply?1:0;
	}
}
static unsigned spins=0,mutateAfter=0;
static void spin(unsigned) { if (mutateAfter && ++spins==mutateAfter) hdmaPage[0x80/4]=3; }
static unsigned sleeps=0,firmwareReadyAfter=0;static bool sleepFails=false;
static int snooze(int) {
	++sleeps;
	if (firmwareReadyAfter && sleeps==firmwareReadyAfter) scratchPage[(0x400+2*4)/4]=5;
	return sleepFails?-1:0;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetMapping.inc"

static void ResetFake() {
	memset(tlbPage,0,sizeof(tlbPage));memset(nodePage,0,sizeof(nodePage));
	memset(resetPage,0,sizeof(resetPage));memset(nocPage,0,sizeof(nocPage));memset(hdmaPage,0,sizeof(hdmaPage));memset(scratchPage,0,sizeof(scratchPage));memset(csmPage,0,sizeof(csmPage));memset(msiPage,0,sizeof(msiPage));
	memset(dbiPage,0,sizeof(dbiPage));dropDbiWrite=blockDbiTlbRestore=false;
	mapCalls=unmapCalls=0;failMap=failUnmap=-1;mapFailureStatus=B_NO_MEMORY;wrongNode=false;
	fakePciPendingPeer=-1;spins=mutateAfter=0;sleeps=firmwareReadyAfter=0;sleepFails=false;blockTlbRestore=false;
	fakeQueueTimeout=fakeQueueBadReply=false;
	cardBars[0]=12;nodePage[0x44/4]=2;resetPage[0x1b0/4]=0x47800;
	scratchPage[(0x400+2*4)/4]=5;
	scratchPage[(0x400+11*4)/4]=0x10001800;
	scratchPage[(0x400+12*4)/4]=0x10001000;
	scratchPage[(0x400+13*4)/4]=0x10000100;
	const uint32 tags[]={28,29,11,36,22,50,34};
	const uint32 values[]={0x12050001,1,1,0xf7,0x5515,0,0xfff};
	csmPage[0x100/4]=0x10000;csmPage[0x104/4]=7;
	for (unsigned i=0;i<7;i++) {
		csmPage[(0x108+4*i)/4]=tags[i]|(i<<16);
		csmPage[(0x1000+4*i)/4]=values[i];
	}
	csmPage[0x1800/4]=0x10028000;csmPage[0x1804/4]=0x404;msiPage[2]=1;
	tlbPage[0x96c/4]=7;tlbPage[0x970/4]=8;tlbPage[0x974/4]=9;
	tlbPage[0x960/4]=10;tlbPage[0x964/4]=11;tlbPage[0x968/4]=12;
}
int main() {
	tt_pioneer_profile profile;PioneerPostResetAuditResult r;tt_arc_scratch_result arc{};
	ResetFake();PioneerPostResetMapping clean(profile);
	assert(clean.Open() && !clean.Audit(r) && !clean.AuditHostDma()
		&& clean.ProbeFirmware(3,arc) && clean.Audit(r) && r.audited==240
		&& clean.AuditHostDma() && clean.Close());
	assert(unmapCalls==7 && tlbPage[0x96c/4]==7 && tlbPage[0x970/4]==8 && tlbPage[0x974/4]==9
		&& tlbPage[0x960/4]==10 && tlbPage[0x964/4]==11 && tlbPage[0x968/4]==12);
	ResetFake();PioneerPostResetMapping queued(profile);tt_arc_queue_result queueResult{};
	assert(queued.Open() && queued.ProbeFirmware(3,arc)
		&& queued.InitializeFirmwareQueue(arc,queueResult) && queued.queueReady
		&& queueResult.submitted && queueResult.acknowledged && queued.Close() && unmapCalls==9);
	ResetFake();PioneerPostResetMapping dbiRestore(profile);
	PioneerRecoveryDbiSnapshot dbiSnapshot{};dbiSnapshot.valid=true;dbiSnapshot.mps=0x40;
	dbiPage[0x78/4]=0x00501002;
	assert(dbiRestore.Open() && dbiRestore.ProbeFirmware(3,arc)
		&& dbiRestore.RestoreInternalDbi(dbiSnapshot,0x2040)
		&& dbiPage[0x78/4]==0x00501042 && !dbiRestore.dbiUnsafe && !dbiRestore.dbiDiagnostic
		&& dbiRestore.Close() && unmapCalls==8 && tlbPage[0x960/4]==10);
	ResetFake();PioneerPostResetMapping dbiLost(profile);
	dbiPage[0x78/4]=0x00501002;dropDbiWrite=true;
	assert(dbiLost.Open() && dbiLost.ProbeFirmware(3,arc)
		&& !dbiLost.RestoreInternalDbi(dbiSnapshot,0x2040)
		&& dbiLost.dbiUnsafe && dbiLost.dbiDiagnostic==5 && dbiLost.dmaModified && !dbiLost.Close() && !unmapCalls);
	ResetFake();PioneerPostResetMapping dbiBusy(profile);
	nocPage[(0x10000+0x240)/4]=1;
	assert(dbiBusy.Open() && dbiBusy.ProbeFirmware(3,arc)
		&& !dbiBusy.RestoreInternalDbi(dbiSnapshot,0x2040) && mapCalls==7);
	assert(dbiBusy.dbiUnsafe && dbiBusy.dbiDiagnostic==2 && !dbiBusy.Close());
	ResetFake();PioneerPostResetMapping dbiDmaBusy(profile);
	hdmaPage[0x100/4]=1;
	assert(dbiDmaBusy.Open() && dbiDmaBusy.ProbeFirmware(3,arc)
		&& !dbiDmaBusy.RestoreInternalDbi(dbiSnapshot,0x2040) && mapCalls==7
		&& dbiDmaBusy.dbiUnsafe && !dbiDmaBusy.Close());
	ResetFake();PioneerPostResetMapping dbiTlbLost(profile);
	assert(dbiTlbLost.Open() && dbiTlbLost.ProbeFirmware(3,arc));blockDbiTlbRestore=true;
	assert(!dbiTlbLost.RestoreInternalDbi(dbiSnapshot,0x2040) && mapCalls==7
		&& dbiTlbLost.dbiUnsafe && dbiTlbLost.dmaModified && !dbiTlbLost.Close());
	for (int failure=7;failure<=8;failure++) {
		ResetFake();PioneerPostResetMapping partialQueue(profile);
		assert(partialQueue.Open() && partialQueue.ProbeFirmware(3,arc));failMap=failure;
		assert(!partialQueue.InitializeFirmwareQueue(arc,queueResult) && !partialQueue.queueReady
			&& !partialQueue.queueUnsafe && partialQueue.Close() && unmapCalls==unsigned(failure));
	}
	ResetFake();PioneerPostResetMapping failedQueue(profile);
	assert(failedQueue.Open() && failedQueue.ProbeFirmware(3,arc));fakeQueueTimeout=true;
	assert(!failedQueue.InitializeFirmwareQueue(arc,queueResult) && failedQueue.queueUnsafe
		&& queueResult.submitted && !queueResult.acknowledged && !failedQueue.Close() && !unmapCalls);
	ResetFake();PioneerPostResetMapping badReply(profile);
	assert(badReply.Open() && badReply.ProbeFirmware(3,arc));fakeQueueBadReply=true;
	assert(!badReply.InitializeFirmwareQueue(arc,queueResult) && badReply.queueUnsafe && !badReply.Close());
	ResetFake();PioneerPostResetMapping interruptedQueue(profile);
	assert(interruptedQueue.Open() && interruptedQueue.ProbeFirmware(3,arc));sleepFails=true;
	assert(!interruptedQueue.InitializeFirmwareQueue(arc,queueResult)
		&& interruptedQueue.queueUnsafe && !queueResult.submitted && !interruptedQueue.Close());
	ResetFake();cardBars[0]=0;PioneerPostResetMapping badBar(profile);
	assert(!badBar.Open() && !mapCalls && badBar.Close());
	ResetFake();failMap=2;PioneerPostResetMapping partial(profile);
	assert(!partial.Open() && partial.Close() && unmapCalls==2);
	for (int failed=0;failed<7;failed++) {
		ResetFake();failMap=failed;PioneerPostResetMapping retry(profile);
		assert(!retry.Open() && retry.PrepareOpenRetry() && unmapCalls==unsigned(failed));
		failMap=-1;assert(retry.Open() && retry.ProbeFirmware(3,arc) && retry.Close());
	}
	ResetFake();failMap=2;failUnmap=0;PioneerPostResetMapping failedCleanup(profile);
	assert(!failedCleanup.Open() && !failedCleanup.PrepareOpenRetry());
	failUnmap=-1;assert(failedCleanup.Close());
	ResetFake();failMap=2;mapFailureStatus=-1;PioneerPostResetMapping otherError(profile);
	assert(!otherError.Open() && !otherError.PrepareOpenRetry() && otherError.Close());
	ResetFake();failMap=2;PioneerPostResetMapping driftDuringRetry(profile);
	assert(!driftDuringRetry.Open());cardBars[0]=0;
	assert(!driftDuringRetry.PrepareOpenRetry() && !unmapCalls);
	cardBars[0]=12;assert(driftDuringRetry.Close());
	ResetFake();nodePage[0x44/4]=3;PioneerPostResetMapping invalidNode(profile);
	assert(!invalidNode.Open() && !invalidNode.PrepareOpenRetry() && invalidNode.Close());
	ResetFake();wrongNode=true;PioneerPostResetMapping wrong(profile);
	assert(wrong.Open() && wrong.ProbeFirmware(3,arc) && !wrong.Audit(r) && r.failure==PioneerPostResetAuditSelect);
	assert(wrong.Close() && tlbPage[0x96c/4]==7);
	ResetFake();nocPage[(0x10000+0x240)/4]=1;PioneerPostResetMapping busy(profile);
	assert(busy.Open() && busy.ProbeFirmware(3,arc) && !busy.Audit(r) && r.failure==PioneerPostResetAuditOutstanding && r.noc==1);
	assert(busy.Close());
	ResetFake();PioneerPostResetMapping retained(profile);
	assert(retained.Open() && retained.ProbeFirmware(3,arc) && retained.SelectPostResetCore(0));
	cardBars[0]=0;assert(!retained.Close() && !unmapCalls);
	cardBars[0]=12;assert(retained.Close());
	ResetFake();PioneerPostResetMapping dmaBusy(profile);
	assert(dmaBusy.Open() && dmaBusy.ProbeFirmware(3,arc) && dmaBusy.Audit(r));
	hdmaPage[0x100/4]=1;assert(!dmaBusy.AuditHostDma());
	assert(dmaBusy.Close() && tlbPage[0x960/4]==10);
	ResetFake();PioneerPostResetMapping pending(profile);
	assert(pending.Open() && pending.ProbeFirmware(3,arc) && pending.Audit(r));
	fakePciPendingPeer=0;assert(!pending.AuditHostDma());
	assert(pending.Close());
	ResetFake();PioneerPostResetMapping rootPending(profile);
	assert(rootPending.Open() && rootPending.ProbeFirmware(3,arc) && rootPending.Audit(r));
	fakePciPendingPeer=1;assert(!rootPending.AuditHostDma());
	assert(rootPending.Close());
	ResetFake();PioneerPostResetMapping changingDma(profile);
	assert(changingDma.Open() && changingDma.ProbeFirmware(3,arc) && changingDma.Audit(r));
	spins=0;mutateAfter=1;assert(!changingDma.AuditHostDma());
	assert(changingDma.Close());
	ResetFake();nodePage[0x44/4]=11;PioneerPostResetMapping otherPcie(profile);
	assert(otherPcie.Open() && otherPcie.ProbeFirmware(3,arc) && otherPcie.Audit(r) && !otherPcie.AuditHostDma());
	assert(otherPcie.Close());
	ResetFake();scratchPage[(0x400+2*4)/4]=0;firmwareReadyAfter=2;
	PioneerPostResetMapping delayed(profile);
	assert(delayed.Open() && delayed.ProbeFirmware(3,arc) && sleeps==2 && delayed.Close());
	ResetFake();scratchPage[(0x400+2*4)/4]=0;PioneerPostResetMapping notReady(profile);
	assert(notReady.Open() && !notReady.ProbeFirmware(2,arc)
		&& sleeps==1 && arc.state==TT_ARC_SCRATCH_NOT_READY && notReady.FirmwareRetryable()
		&& !notReady.Audit(r));
	assert(!notReady.ProbeFirmware(2,arc) && notReady.FirmwareRetryable() && sleeps==2);
	scratchPage[(0x400+2*4)/4]=5;
	assert(notReady.ProbeFirmware(2,arc) && !notReady.FirmwareRetryable() && notReady.Close());
	ResetFake();scratchPage[(0x400+2*4)/4]=0;PioneerPostResetMapping drift(profile);
	assert(drift.Open() && !drift.ProbeFirmware(1,arc) && drift.FirmwareRetryable());
	tlbPage[0x96c/4]=6;
	assert(!drift.ProbeFirmware(1,arc) && !drift.FirmwareRetryable());
	tlbPage[0x96c/4]=7;
	assert(!drift.ProbeFirmware(1,arc) && drift.Close());
	ResetFake();scratchPage[(0x400+2*4)/4]=0;sleepFails=true;
	PioneerPostResetMapping interrupted(profile);
	assert(interrupted.Open() && !interrupted.ProbeFirmware(3,arc) && sleeps==1
		&& !interrupted.FirmwareRetryable() && !interrupted.ProbeFirmware(3,arc) && interrupted.Close());
	ResetFake();scratchPage[(0x400+4*4)/4]=2;PioneerPostResetMapping fwError(profile);
	assert(fwError.Open() && !fwError.ProbeFirmware(1,arc) && fwError.Close());
	ResetFake();csmPage[0x1000/4]=0x13000000;PioneerPostResetMapping wrongFirmware(profile);
	assert(wrongFirmware.Open() && !wrongFirmware.ProbeFirmware(1,arc)
		&& !wrongFirmware.Audit(r) && wrongFirmware.Close());
	ResetFake();PioneerPostResetMapping badRestore(profile);
	assert(badRestore.Open());blockTlbRestore=true;
	assert(!badRestore.ProbeFirmware(3,arc) && arc.state==TT_ARC_SCRATCH_RESTORE_FAILED
		&& !badRestore.FirmwareRetryable() && !sleeps && !badRestore.Close() && !unmapCalls);
	blockTlbRestore=false;assert(badRestore.Close());
	puts("PASS post-reset ARC scratch/telemetry, fleet mapping and DMA gates; SIMULATED MMIO ONLY");
}
