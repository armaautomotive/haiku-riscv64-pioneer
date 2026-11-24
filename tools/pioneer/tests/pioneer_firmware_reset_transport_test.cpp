/* SPDX-License-Identifier: MIT */
#define PIONEER_ARC_QUEUE_MODEL_ONLY
#include "pioneer_arc_queue_probe_test.cpp"
#include <cstdio>
#include <cstring>

using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using area_id=int;using status_t=int;
enum { B_OK=0,B_PAGE_SIZE=4096,B_ANY_KERNEL_ADDRESS=0,B_UNCACHED_MEMORY=1,
	B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_BAD_VALUE=-2,B_NOT_ALLOWED=-3,B_DEV_NOT_READY=-4 };
static uint32 tlbs[6],queueWords[TT_ARC_QUEUE_CSM_BYTES/4],msi[1024],preparedMsi[1024];
static unsigned maps=0,queuePublishes=0,resetBells=0,pingBells=0,afterPublishMmio=0;
static int failMap=-1,dropTlb=-1;static bool published=false,firmwareCompletes=false;
static bool leaseValid=true;static unsigned validateCalls=0,denyAt=0;static uint32 cardCommand=0x402;
static int deleted[10],deletionCount=0,failDelete=-1,aliasMap=-1;
static constexpr uint64 Bar=0x100000000ULL;
struct Profile { uint64 bar0_host_start=Bar; };
struct PioneerPostResetMapping {
	Profile profile;bool ready=true,firmwareReady=true,topologyReady=true,savedValid=true;
	bool queueAttempted=false,queueUnsafe=false,dmaModified=false,dbiAttempted=false,modified=false;
	area_id areas[7]={20,21,22,23,24,25,26},queueArea=-1,queueMsiArea=-1,dbiArea=-1;
	uint32 dmaSaved[3]={10,11,12};volatile uint32* tlb=tlbs+3;
	const volatile uint32 *node=nullptr,*reset=nullptr,*noc=nullptr,*hdma=nullptr,*scratch=nullptr,*csm=nullptr;
	bool PciReady(bool resetMarker=false) { return cardCommand==(resetMarker?0x442U:0x402U); }
	uint32 TlbRead(unsigned w) { return PioneerRead32(tlb+w); }
	void TlbWrite(unsigned w,uint32 value) { PioneerWrite32(tlb+w,value); }
	static uint32 PioneerRead32(const volatile uint32* p) { return *p; }
	static void PioneerWrite32(volatile uint32* p,uint32 value) { *p=value; }
};
struct Guard { int (*validate)(uint64); };
static int Validate(uint64 t) {
	if (++validateCalls==denyAt) leaseValid=false;
	return t==123 && leaseValid?B_OK:-1;
}
static bool Within(const volatile uint32* p,const uint32* start,size_t words) {
	const uintptr_t a=(uintptr_t)p,b=(uintptr_t)start;
	return a>=b && a<b+words*sizeof(uint32);
}
static int map_physical_memory(const char*,uint64 addr,size_t size,int spec,uint32 flags,void** out) {
	assert(spec==(B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY));
	assert(flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA));
	const unsigned n=maps++;if (int(n)==failMap) return -1;
	if (addr==Bar+0x19200000 && size==TT_ARC_QUEUE_CSM_BYTES) *out=queueWords;
	else if (addr==Bar+0x192b0000 && size==B_PAGE_SIZE) *out=msi;
	else if (addr==Bar+0x190b0000 && size==B_PAGE_SIZE) *out=preparedMsi;
	else assert(false);
	return int(n)==aliasMap?20:int(n);
}
static int delete_area(area_id id) {
	assert(deletionCount<10);deleted[deletionCount++]=id;
	return id==failDelete?-5:B_OK;
}
static uint32 PioneerRead32(const volatile uint32* p) {
	assert(leaseValid);
	if (published) ++afterPublishMmio;
	if (Within(p,queueWords,TT_ARC_QUEUE_CSM_BYTES/4))
		assert(tlbs[3]==0x80 && tlbs[4]==0x04004000 && tlbs[5]==0x40);
	if (Within(p,msi,1024)) assert(tlbs[3]==0x400 && tlbs[4]==0x04004000 && tlbs[5]==0x40);
	if (Within(p,preparedMsi,1024)) assert(tlbs[0]==0x400 && tlbs[1]==0x04004000 && tlbs[2]==0x40);
	return *p;
}
static void PioneerWrite32(volatile uint32* p,uint32 value) {
	assert(leaseValid);
	if (published && p!=preparedMsi) ++afterPublishMmio;
	if (Within(p,tlbs,3) && int(p-tlbs)==dropTlb) return;
	if (Within(p,queueWords,TT_ARC_QUEUE_CSM_BYTES/4)) {
		assert(tlbs[3]==0x80 && tlbs[4]==0x04004000 && tlbs[5]==0x40);
		if (p==queueWords+0x28000/4 && tlbs[0]==0x400) { ++queuePublishes;published=true; }
	}
	if (p==msi) {
		assert(tlbs[3]==0x400 && !published);++pingBells;
		const unsigned b=0x28000/4,req=queueWords[b],resp=queueWords[b+1];
		assert(queueWords[b+8+((req+7)%8%4)*8]==0x90);
		queueWords[b+4]=req;queueWords[b+5]=(resp+1)%8;queueWords[b+6]++;
		queueWords[b+40+(resp%4)*8]=0;
	}
	if (p==preparedMsi) { assert(published && value==0);++resetBells; }
	*p=value;
}
static int snooze(int delay) {
	if (delay==100000 && firmwareCompletes) cardCommand=0x402;
	return B_OK;
}
struct Owner {
	uint32 CardRead(uint16 a,uint8 s) {
		assert(leaseValid);
		if (a==0 && s==4) return 0xb1401e52;
		if (a==4 && s==2) return cardCommand;
		assert(false);return 0;
	}
	uint32 RootRead(uint16 a,uint8 s) {
		assert(leaseValid);
		if (a==0 && s==4) return 0x20421e30;
		if (a==8 && s==4) return 0x06040000;
		if (a==4 && s==2) return 0x402;
		assert(false);return 0;
	}
	void CardWrite(uint16 a,uint8 s,uint32 v) { assert(leaseValid && a==4 && s==2 && v==0x442);cardCommand=v; }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerFirmwareResetTransport.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerFirmwareRecoveryOwner.h"

static PioneerFirmwareResetPollTrace pollEvents[3];
static unsigned pollEventCount=0;
static void RecordPoll(void* context,PioneerFirmwareResetPollTrace event) {
	assert(context==&pollEventCount && pollEventCount<3 && published && resetBells==1);
	pollEvents[pollEventCount++]=event;
}
static void ResetFake() {
	memset(tlbs,0,sizeof(tlbs));memset(queueWords,0,sizeof(queueWords));
	memset(msi,0,sizeof(msi));memset(preparedMsi,0,sizeof(preparedMsi));
	tlbs[0]=10;tlbs[1]=11;tlbs[2]=12;tlbs[3]=1;tlbs[4]=2;tlbs[5]=3;
	queueWords[0x1000/4]=0x10028000;queueWords[0x1004/4]=0x404;
	msi[2]=1;preparedMsi[2]=1;maps=queuePublishes=resetBells=pingBells=afterPublishMmio=0;
	failMap=dropTlb=failDelete=aliasMap=-1;deletionCount=0;published=firmwareCompletes=false;
	leaseValid=true;validateCalls=denyAt=0;cardCommand=0x402;
	pollEventCount=0;
}
static PioneerDriverRecoveryProof Evidence() {
	PioneerDriverRecoveryProof p{};
	p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
	p.core_count=120;p.retained_owners=1;p.reset_generation=p.firmware_generation=123;return p;
}
int main() {
	const auto arc=MockQueue::Arc();const auto health=MockQueue::Health();
	Guard guard{Validate};Owner owner;
	ResetFake();PioneerPostResetMapping m;
	PioneerFirmwareResetTransport<Owner,Guard> io(m,owner,&guard,123);
	assert(io.Open() && m.queueUnsafe && !io.Open());
	PioneerFirmwareReset<decltype(io)> reset(io);
	const bool started=reset.Start(arc,health);
	if (!started || pingBells!=1 || queuePublishes!=1 || resetBells!=1) std::fprintf(stderr,"stage=%u failed=%u probe_state=%u ping=%u pubs=%u bells=%u\n",
		reset.Result().stage,reset.Result().failedStage,reset.Result().probe.state,pingBells,queuePublishes,resetBells);
	assert(started && pingBells==1 && queuePublishes==1 && resetBells==1);
	const unsigned startChecks=validateCalls;
	assert(m.dmaModified && !afterPublishMmio && !reset.ResumeWait(1));
	assert(io.DisposeAfterVerifiedReset(Evidence(),reset)==B_BAD_VALUE && !deletionCount);
	io.DetachGuard();assert(!reset.ResumeWait(1));
	assert(!io.BindGuard(&guard,124) && !reset.ResumeWait(1));
	leaseValid=false;assert(!io.BindGuard(&guard,123));leaseValid=true;
	assert(io.BindGuard(&guard,123));
	firmwareCompletes=true;assert(reset.ResumeWait(1) && !afterPublishMmio);
	assert(!reset.Start(arc,health) && queuePublishes==1 && resetBells==1);
	auto badProof=Evidence();badProof.firmware_generation=124;
	assert(io.DisposeAfterVerifiedReset(badProof,reset)==B_BAD_VALUE && !deletionCount);
	PioneerFirmwareReset<decltype(io)> wrongReset(io);
	assert(io.DisposeAfterVerifiedReset(Evidence(),wrongReset)==B_BAD_VALUE && !deletionCount);
	assert(io.DisposeAfterVerifiedReset(Evidence(),reset)==B_OK && io.DisposalComplete());
	assert(deletionCount==10 && deleted[0]==2 && deleted[1]==1 && deleted[2]==0
		&& deleted[3]==20 && deleted[9]==26 && !m.ready && !m.tlb && !afterPublishMmio);
	assert(io.DisposeAfterVerifiedReset(Evidence(),reset)==B_NOT_ALLOWED && deletionCount==10);
	assert(!io.BindGuard(&guard,123));
	for (int n=0;n<3;n++) {
		ResetFake();failMap=n;PioneerPostResetMapping partial;
		PioneerFirmwareResetTransport<Owner,Guard> t(partial,owner,&guard,123);
		assert(!t.Open() && partial.queueUnsafe && !t.Open() && maps==unsigned(n+1));
	}
	for (int n=0;n<3;n++) {
		ResetFake();dropTlb=n;PioneerPostResetMapping fail;
		PioneerFirmwareResetTransport<Owner,Guard> t(fail,owner,&guard,123);
		assert(t.Open());PioneerFirmwareReset<decltype(t)> r(t);
		assert(!r.Start(arc,health) && fail.queueUnsafe && fail.dmaModified && !queuePublishes && !resetBells);
	}
	for (unsigned n=1;n<=startChecks;n++) {
		ResetFake();denyAt=n;PioneerPostResetMapping retained;
		PioneerFirmwareResetTransport<Owner,Guard> candidate(retained,owner,&guard,123);
		if (candidate.Open()) {
			PioneerFirmwareReset<decltype(candidate)> r(candidate);
			(void)r.Start(arc,health);
			assert(retained.queueUnsafe && !leaseValid && queuePublishes<=1 && resetBells<=1);
			assert(!r.Start(arc,health) && queuePublishes<=1 && resetBells<=1);
		} else assert(!leaseValid && !queuePublishes && !resetBells);
	}
	ResetFake();tlbs[0]=99;PioneerPostResetMapping drift;
	PioneerFirmwareResetTransport<Owner,Guard> d(drift,owner,&guard,123);
	assert(d.Open());PioneerFirmwareReset<decltype(d)> dr(d);
	assert(!dr.Start(arc,health) && drift.queueUnsafe && !drift.dmaModified && !queuePublishes);
	ResetFake();preparedMsi[2]=0;PioneerPostResetMapping noBell;
	PioneerFirmwareResetTransport<Owner,Guard> b(noBell,owner,&guard,123);
	assert(b.Open());PioneerFirmwareReset<decltype(b)> br(b);
	assert(!br.Start(arc,health) && noBell.queueUnsafe && noBell.dmaModified && !queuePublishes);
	ResetFake();aliasMap=1;PioneerPostResetMapping alias;
	PioneerFirmwareResetTransport<Owner,Guard> a(alias,owner,&guard,123);
	assert(a.Open());PioneerFirmwareReset<decltype(a)> ar(a);
	assert(ar.Start(arc,health));firmwareCompletes=true;assert(ar.ResumeWait(1));
	assert(a.DisposeAfterVerifiedReset(Evidence(),ar)==B_BAD_VALUE && !deletionCount);
	for (int failing=0;failing<10;failing++) {
		ResetFake();PioneerPostResetMapping partial;
		PioneerFirmwareResetTransport<Owner,Guard> c(partial,owner,&guard,123);
		assert(c.Open());PioneerFirmwareReset<decltype(c)> cr(c);
		assert(cr.Start(arc,health));firmwareCompletes=true;assert(cr.ResumeWait(1));
		failDelete=failing<3?2-failing:20+failing-3;
		assert(c.DisposeAfterVerifiedReset(Evidence(),cr)==-5 && deletionCount==failing+1);
		assert(!c.DisposalComplete() && c.DisposeAfterVerifiedReset(Evidence(),cr)==B_NOT_ALLOWED
			&& deletionCount==failing+1 && !afterPublishMmio);
	}
	ResetFake();PioneerPostResetMapping losingLease;
	PioneerFirmwareResetTransport<Owner,Guard> l(losingLease,owner,&guard,123);
	assert(l.Open());PioneerFirmwareReset<decltype(l)> lr(l);
	assert(lr.Start(arc,health));firmwareCompletes=true;assert(lr.ResumeWait(1));
	denyAt=validateCalls+3; // proof check, first unmap, then ticket loss
	assert(l.DisposeAfterVerifiedReset(Evidence(),lr)==B_DEV_NOT_READY && deletionCount==1);
	assert(l.DisposeAfterVerifiedReset(Evidence(),lr)==B_NOT_ALLOWED && deletionCount==1);
	for (unsigned n=0;n<4;n++) {
		ResetFake();PioneerPostResetMapping busy;
		if (n==0) busy.dbiAttempted=true;else if (n==1) busy.dbiArea=99;
		else if (n==2) busy.queueArea=99;else busy.dmaModified=true;
		PioneerFirmwareResetTransport<Owner,Guard> blocked(busy,owner,&guard,123);
		assert(!blocked.Open() && !maps);
	}
	ResetFake();PioneerPostResetMapping owned;
	PioneerFirmwareRecoveryOwner<Owner,Guard> recovery(owned,owner,123);
	assert(!recovery.SetTrace(nullptr,&pollEventCount));
	assert(recovery.SetTrace(RecordPoll,&pollEventCount) && !recovery.SetTrace(RecordPoll,&pollEventCount));
	assert(!recovery.Start(&guard,124,arc,health) && !maps);
	leaseValid=false;assert(!recovery.Start(&guard,123,arc,health) && !maps);leaseValid=true;
	assert(recovery.Start(&guard,123,arc,health) && queuePublishes==1 && resetBells==1);
	assert(!recovery.Start(&guard,123,arc,health) && !recovery.SetTrace(RecordPoll,&pollEventCount));
	assert(!recovery.Resume(&guard,124,1) && !recovery.Resume(&guard,123,1));
	assert(pollEventCount==3 && pollEvents[0]==PioneerFirmwareResetBeforeFirstPoll
		&& pollEvents[1]==PioneerFirmwareResetAfterFirstVendor
		&& pollEvents[2]==PioneerFirmwareResetAfterFirstMarker);
	firmwareCompletes=true;assert(recovery.Resume(&guard,123,1));
	assert(pollEventCount==3); // retry does not duplicate evidence or reset
	badProof=Evidence();badProof.reset_generation=124;
	assert(recovery.Retire(&guard,123,badProof)==B_BAD_VALUE && !deletionCount);
	assert(recovery.Retire(&guard,123,Evidence())==B_OK && recovery.Retired() && deletionCount==10);
	assert(!recovery.Resume(&guard,123,1) && queuePublishes==1 && !afterPublishMmio);
	ResetFake();leaseValid=false;PioneerPostResetMapping denied;
	PioneerFirmwareResetTransport<Owner,Guard> t(denied,owner,&guard,123);
	assert(!t.Open() && !maps && !denied.queueUnsafe);
	puts("PASS actual experimental firmware-reset transport: separate TLBs, guarded one-shot publication and retained failures; SIMULATED ONLY");
}
