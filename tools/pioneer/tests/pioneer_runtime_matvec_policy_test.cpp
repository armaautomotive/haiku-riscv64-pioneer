/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeMatvec.h"
struct RuntimeMock:StoreMock {
	uint32_t code[1025]={},buffers[8192]={},saved[9217]={},stage[1024]={},status[3]={};
	uint32_t reset=0x47800;
	unsigned starts=0,stops=0;
	bool executeTimeout=false,badPc=false,badMathFlits=false,stopFails=false,restoreCodeFails=false,restoreBufferFails=false,nonfinite=false;
	RuntimeMock() {
		for (unsigned w=0;w<1025;w++) code[w]=0x12340000+w;
		for (unsigned w=0;w<8192;w++) buffers[w]=0xabc00000+w;
	}
	uint32_t ResetRead() { return reset; }
	uint32_t* RuntimeSavedWords() { return saved; }
	uint32_t* RuntimeStageWords() { return stage; }
	uint32_t RuntimeCodeRead(unsigned w) { assert(w<1025); return code[w]; }
	void RuntimeCodeWrite(unsigned w,uint32_t value) {
		assert(w<1025 && !uncertain && reset==0x47800);
		if (++Mock::writes!=Mock::drop && !(restoreCodeFails && value==0x12340000+w)) code[w]=value;
	}
	uint32_t RuntimeBufferRead(unsigned w) { assert(w<8192); return buffers[w]; }
	void RuntimeBufferWrite(unsigned w,uint32_t value) {
		assert(w<8192 && !uncertain && reset==0x47800);
		if (++Mock::writes!=Mock::drop && !(restoreBufferFails && value==0xabc00000+w)) buffers[w]=value;
	}
	uint32_t RuntimeControlRead(unsigned w) { assert(w<3); return 0; }
	uint32_t RuntimeStatusRead(unsigned w) { assert(w<3); return status[w]; }
	void RuntimeStatusWrite(unsigned w,uint32_t value) {
		assert(w<3 && reset==0x47800 && !uncertain);
		if (++Mock::writes!=Mock::drop) status[w]=value;
	}
	uint32_t RuntimePcRead() { return badPc?0x70000:TT_RUNTIME_MATVEC_PARK_PC; }
	void RuntimeResetWrite(uint32_t value) {
		assert(value==0x87ffff || value==0x46800 || value==0x47800);
		if (reset==0x46800 && value==0x87ffff) { assert(!executeTimeout); stops++; }
		if (++Mock::writes==Mock::drop || (stopFails && starts && value==0x47800)) return;
		reset=value;
		if (value==0x46800) {
			starts++; if (executeTimeout) return;
			unsigned tiles=code[1+0x504/4]*code[1+0x508/4];
			status[0]=TT_RUNTIME_MATVEC_FINAL_PHASE; status[1]=tiles; status[2]=0x4d415431;
			counters[0]+=tiles; counters[1]+=tiles*9+(badMathFlits?1:0); accepted+=tiles; counters[3]+=tiles;
			for (unsigned w=0;w<code[1+0x508/4]*16;w++) buffers[4096+w]=nonfinite?0x7fc00000:0x40000000;
		}
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset==0x40 && command[3]>=0x80000 && command[3]<0x88000) {
			assert(!uncertain && !pending && value==1 && command[7]==0 && command[8]==4096);
			assert(command[0]==uint32_t(physical) && command[1]==uint32_t(physical>>32) && command[2]==(19|(24<<6)));
			doorbells++; doorbellRead=false;
			if (++Mock::writes==Mock::drop) { uncertain=true; return; }
			requests++; assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
			if (failRequest==doorbells) { pending=uncertain=true; return; }
			for (unsigned w=0;w<1024;w++) buffers[(command[3]-0x80000)/4+w]=page[w];
			counters[0]++; counters[1]+=64; accepted++; counters[3]++; done=true; return;
		}
		StoreMock::ResidentNocWrite(offset,value);
	}
	void Restored() {
		assert(reset==0x47800);
		for (unsigned w=0;w<1025;w++) assert(code[w]==0x12340000+w);
		for (unsigned w=0;w<8192;w++) assert(buffers[w]==0xabc00000+w);
	}
};
static bool Run(RuntimeMock& io,tt_host_page_result& host,PioneerResidentLease& store,PioneerRuntimeMatvecLease& compute,
	tt_runtime_matvec_result& r)
{
	auto b=io.Baseline(); auto n=io.Compute(); uint32_t input[512],output[32]={};
	r={};
	for (auto& value:input) value=0x3f803f80;
	if (!PioneerRuntimeMatvecPrepare(io,b,n,store,compute)) return false;
	return PioneerRuntimeMatvec(io,b,n,host,store,compute,73728,0,1024,32,input,output,r);
}
int main()
{
	RuntimeMock io; tt_host_page_result host; PioneerResidentLease store; PioneerRuntimeMatvecLease compute; tt_runtime_matvec_result r;
	assert(Prepared(io,host,store) && Run(io,host,store,compute,r));
	unsigned runtimeWrites=io.Mock::writes;
	assert(r.started && r.completed && r.stopped && r.staged_pages==2 && !r.recovery_required && io.starts==1 && io.stops==1);
	assert(Run(io,host,store,compute,r) && io.starts==2);
	auto b=io.Baseline(); auto n=io.Compute();
	assert(PioneerRuntimeMatvecClose(io,b,n,store,compute)); io.Restored();
	assert(PioneerResidentLeaseEnd(io,n,store) && io.ReleaseHostPage()==0); io.Safe();
	for (unsigned fault=0;fault<7;fault++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s));
		trial.executeTimeout=fault==0; trial.badPc=fault==1; trial.badMathFlits=fault==2; trial.stopFails=fault==3;
		trial.failRequest=fault==4?1:0; trial.clearFails=fault==5; trial.changed=fault==6;
		assert(!Run(trial,h,s,c,r));
		if (fault!=6) assert(r.recovery_required && trial.quarantined);
		if (fault<3) assert(trial.stops==0);
		if (r.recovery_required) assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	for (unsigned fault=0;fault<2;fault++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r));
		trial.restoreCodeFails=fault==0; trial.restoreBufferFails=fault==1;
		assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c) && s.recovery_required);
	}
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); trial.nonfinite=true;
		assert(!Run(trial,h,s,c,r) && r.completed && r.stopped && r.nonfinite==32 && !r.recovery_required);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	// Sample interior code writes; then drop every write after the fixed
	// code upload, including all staging/control/doorbell/stop boundaries.
	// Full image equality independently catches a lost program upload.
	const unsigned droppedWrites[]={8,9,13,200,900,1032,1040,1050,1060,1080};
	for (unsigned drop:droppedWrites) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); trial.Mock::drop=drop;
		Run(trial,h,s,c,r);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c));
			assert(PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		}
	}
	unsigned controlCases=0;
	for (unsigned drop=0;drop<=runtimeWrites;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s));
		if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; controlCases++;
		Run(trial,h,s,c,r);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c));
			assert(PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	assert(controlCases>20);
	printf("Dropped every post-upload control write: %u cases\n",controlCases);
}
