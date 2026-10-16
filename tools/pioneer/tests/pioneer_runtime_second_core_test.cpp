/* SPDX-License-Identifier: MIT */
#include <initializer_list>
#define PIONEER_RUNTIME_POLICY_MOCK_ONLY
#include "pioneer_runtime_matvec_policy_test.cpp"
// The existing single-core model has one physical memory image. Translate
// only its PCIe window encoding, checking every real second-core destination
// before normalizing it for the old model. Never normalize payload/math.
struct SecondRuntimeMock:RuntimeMock {
	unsigned destinations=0;
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
		if (word==1 && value==BlackholeDramFixedTlbWord(0,1,3,false,1))
			value=BlackholeDramFixedTlbWord(0,1,2,false,1);
		StoreMock::DramTlbWrite(slot,word,value);
	}
	uint32_t DramTlbRead(unsigned slot,unsigned word) {
		uint32_t value=StoreMock::DramTlbRead(slot,word);
		if (word==1 && value==BlackholeDramFixedTlbWord(0,1,2,false,1))
			value=BlackholeDramFixedTlbWord(0,1,3,false,1);
		return value;
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset==20 && value) {
			assert(value==(1|(3<<6))); destinations++;
			value=1|(2<<6);
		}
		RuntimeMock::ResidentNocWrite(offset,value);
	}
	uint32_t ResidentNocRead(uint32_t offset) {
		uint32_t value=RuntimeMock::ResidentNocRead(offset);
		return offset==20 && value==(1|(2<<6))?uint32_t(1|(3<<6)):value;
	}
};
static bool SecondRun(SecondRuntimeMock& io,tt_host_page_result& h,PioneerResidentLease& s,
	PioneerRuntimeMatvecLease& c,tt_runtime_matvec_result& r)
{
	auto b=io.Baseline(); auto n=io.Compute(); n.version=2;
	uint32_t input[512],scales[32],output[32]={};
	for (auto& v:input) v=0x40014001;
	for (auto& v:scales) v=0x3c800000;
	return PioneerRuntimeMatvecPrepare(io,b,n,s,c,false,true,true,true,true,true,true)
		&& PioneerRuntimeMatvec(io,b,n,h,s,c,36864,0,1024,32,input,output,r,false,scales,3);
}
static bool SecondPrepared(SecondRuntimeMock& io,tt_host_page_result& h,PioneerResidentLease& s)
{
	io.bufferWords=12288; auto n=io.Compute(); n.version=2;
	return PioneerPrepareHostPage(io,h) && PioneerResidentLeaseBegin(io,n,s,3);
}
static void SecondClose(SecondRuntimeMock& io,tt_host_page_result& h,PioneerResidentLease& s,
	PioneerRuntimeMatvecLease& c,tt_runtime_matvec_result& r)
{
	(void)h; auto b=io.Baseline(); auto n=io.Compute(); n.version=2;
	if (r.recovery_required || c.unsafe) {
		assert(!PioneerRuntimeMatvecClose(io,b,n,s,c));
		assert(!PioneerResidentLeaseEnd(io,n,s)); return;
	}
	assert(PioneerRuntimeMatvecClose(io,b,n,s,c) && PioneerResidentLeaseEnd(io,n,s));
	io.Restored(); assert(io.ReleaseHostPage()==0); io.Safe();
}
#ifndef PIONEER_SECOND_CORE_MOCK_ONLY
int main()
{
	SecondRuntimeMock io; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
	tt_runtime_matvec_result r={};
	assert(SecondPrepared(io,h,s));
	bool passed=SecondRun(io,h,s,c,r);
	if (!passed) fprintf(stderr,"Second core model state %u prepared %u ready %u started/completed/stopped %u/%u/%u cold %u saved %u dest %u\n",r.state,r.prepared,c.ready,r.started,r.completed,r.stopped,r.recovery_required,c.saved,io.destinations);
	assert(passed);
	unsigned writes=io.Mock::writes;
	assert(io.destinations==4 && r.staged_pages==3 && r.completed && r.stopped && !r.recovery_required);
	assert(s.l1_dirty && !s.l1_restored); // completion words also require tail restoration
	assert(SecondRun(io,h,s,c,r) && io.destinations==8);
	SecondClose(io,h,s,c,r);
	unsigned cases=0;
	for (unsigned drop=0;drop<=writes;drop++) {
		SecondRuntimeMock t; tt_host_page_result page; PioneerResidentLease lease; PioneerRuntimeMatvecLease compute;
		assert(SecondPrepared(t,page,lease));
		if (drop<t.Mock::writes+1026) continue;
		t.Mock::drop=drop; cases++; r={}; SecondRun(t,page,lease,compute,r);
		SecondClose(t,page,lease,compute,r);
	}
	for (unsigned fault=0;fault<7;fault++) {
		SecondRuntimeMock t; tt_host_page_result page; PioneerResidentLease lease; PioneerRuntimeMatvecLease compute;
		assert(SecondPrepared(t,page,lease));
		t.executeTimeout=fault==0; t.badPc=fault==1; t.badMathFlits=fault==2; t.stopFails=fault==3;
		t.failRequest=fault==4?2:0; t.clearFails=fault==5; t.changed=fault==6;
		r={}; assert(!SecondRun(t,page,lease,compute,r));
		if (fault!=6) assert(r.recovery_required && t.quarantined);
		if (fault<3) assert(!t.stops);
		SecondClose(t,page,lease,compute,r);
	}
	for (unsigned y:{0U,1U,4U,63U}) {
		SecondRuntimeMock t; PioneerResidentLease lease; auto n=t.Compute(); n.version=2;
		assert(!PioneerResidentLeaseBegin(t,n,lease,y) && !t.Mock::writes);
	}
	printf("PASS: fixed second-core destination, exact bulk lease, completion-tail rollback and %u dropped control writes; not native or parallel\n",cases);
}
#endif
