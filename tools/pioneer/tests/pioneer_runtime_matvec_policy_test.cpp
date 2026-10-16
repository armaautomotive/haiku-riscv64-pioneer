/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeMatvec.h"
struct RuntimeMock:StoreMock {
	PioneerRuntimeProfile runtimeProfile;
	uint64_t microseconds=100;
	void RuntimeProfileMark(unsigned slot) { runtimeProfile.Mark(microseconds+=50,slot); }
	uint32_t code[1025]={},buffers[8192]={},saved[9217]={},stage[1024]={},status[3]={};
	uint32_t reset=0x47800;
	unsigned starts=0,stops=0,outputPioReads=0,outputDmaWrites=0;
	bool executeTimeout=false,badPc=false,badMathFlits=false,stopFails=false,restoreCodeFails=false,restoreBufferFails=false,nonfinite=false;
	bool omitDmaPayload=false;
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
	uint32_t RuntimeBufferRead(unsigned w) { assert(w<8192); if (w>=4096) outputPioReads++; return buffers[w]; }
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
	bool Q8() const { return code[1+0xb00/4]!=0; }
	bool Compact() const { return code[1+0xe00/4]!=0; }
	uint32_t RuntimePcRead() { return badPc?0x70000:Q8()?TT_RUNTIME_Q8_PARK_PC:TT_RUNTIME_MATVEC_PARK_PC; }
	void RuntimeResetWrite(uint32_t value) {
		assert(value==0x87ffff || value==0x46800 || value==0x47800);
		if (reset==0x46800 && value==0x87ffff) { assert(!executeTimeout); stops++; }
		if (++Mock::writes==Mock::drop || (stopFails && starts && value==0x47800)) return;
		reset=value;
		if (value==0x46800) {
			starts++; if (executeTimeout) return;
			unsigned tiles=code[1+0x504/4]*code[1+0x508/4];
			unsigned reads=Compact()?tiles/2:tiles;
			assert(command[3]==(Compact()?0x17e000U:0x70700U));
			status[0]=Q8()?TT_RUNTIME_Q8_FINAL_PHASE:TT_RUNTIME_MATVEC_FINAL_PHASE; status[1]=tiles; status[2]=0x4d415431;
			counters[0]+=reads; counters[1]+=reads*9+(badMathFlits?1:0); accepted+=reads; counters[3]+=reads;
			for (unsigned w=0;w<code[1+0x508/4]*16;w++) buffers[4096+w]=code[42]==0xa02202b7 && !Q8()?uint32_t(-11282):nonfinite?0x7fc00000:0x40000000;
		}
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset==0x40 && ((command[7]==0x212 && command[5]==(19|(24<<6)))
			|| (command[2]==(19|(24<<6)) && command[3]==0x17f000))) {
			assert(reset==0x47800 && stops && !uncertain && !pending && value==1 && command[8]==4096);
			assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
			doorbells++; doorbellRead=false;
			if (++Mock::writes==Mock::drop) { uncertain=true; return; }
			requests++; pending=true;
			if (failRequest==doorbells) { uncertain=true; return; }
			if (command[7]==0x212) {
				assert(command[0]>=0x84000 && command[0]<=0x87000 && command[0]%4096==0);
				outputDmaWrites++;
				if (!omitDmaPayload) for (unsigned w=0;w<1024;w++) page[w]=buffers[(command[0]-0x80000)/4+w];
				writeCounters[0]++; writeCounters[1]+=64; accepted++; writeCounters[3]++;
			} else {
				for (unsigned w=0;w<1024;w++) destination[1024+w]=page[w];
				counters[0]++; counters[1]+=64; accepted++; counters[3]++;
			}
			pending=false; done=true; return;
		}
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
	tt_runtime_matvec_result& r,bool dma=false,bool fp16=false,bool int8=false,bool q8=false,bool compact=false)
{
	auto b=io.Baseline(); auto n=io.Compute(); uint32_t input[512],output[32]={};
	r={};
	for (auto& value:input) value=int8?0x40014001:0x3f803f80;
	uint32_t scales[32]; for (auto& value:scales) value=0x3c800000;
	io.runtimeProfile.Begin(io.microseconds,compact?5:q8?4:int8?3:fp16?2:dma?1:0);
	bool ready=PioneerRuntimeMatvecPrepare(io,b,n,store,compute,fp16,int8,q8,compact),passed=false;
	if (ready) {
		io.RuntimeProfileMark(TT_PROFILE_PREPARED);
		passed=PioneerRuntimeMatvec(io,b,n,host,store,compute,compact?36864:73728,0,1024,32,input,output,r,dma,q8?scales:NULL);
	}
	io.runtimeProfile.End(io.microseconds+=50,r.state,r.recovery_required,r.started,r.completed,r.stopped);
	const auto& p=io.runtimeProfile.Snapshot();
	assert(p.available && p.clock_stable && p.state==r.state && p.recovery_required==r.recovery_required);
	assert(p.started==r.started && p.completed==r.completed && p.stopped==r.stopped);
	if (passed) assert(p.mask==(dma?0x1dfU:0x17fU) && p.format==(compact?5U:q8?4U:int8?3U:fp16?2U:dma?1U:0U));
	if (passed) for (uint32_t v:output) assert(v==(int8 && !q8?uint32_t(-11282):0x40000000));
	return passed;
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
	unsigned dmaWrites=0;
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r,true)); dmaWrites=trial.Mock::writes;
		assert(r.stopped && !r.recovery_required && trial.outputDmaWrites==1 && trial.outputPioReads==4096);
		assert(Run(trial,h,s,c,r,true) && trial.outputDmaWrites==2 && trial.outputPioReads==4096);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c)); trial.Restored();
		assert(PioneerResidentLeaseEnd(trial,n,s) && trial.ReleaseHostPage()==0); trial.Safe();
	}
	unsigned dmaCases=0;
	for (unsigned drop=0;drop<=dmaWrites;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; dmaCases++;
		Run(trial,h,s,c,r,true);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	for (unsigned fault=0;fault<5;fault++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s));
		trial.omitDmaPayload=fault==0; trial.failRequest=fault>=1 && fault<=3?fault+2:0; trial.stopFails=fault==4;
		assert(!Run(trial,h,s,c,r,true) && r.recovery_required && trial.quarantined && !trial.releases);
		unsigned stops=trial.stops;
		assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c) && trial.stops==stops);
		if (fault==4) assert(!trial.outputDmaWrites);
	}
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); trial.nonfinite=true;
		assert(!Run(trial,h,s,c,r,true) && r.state==TT_RUNTIME_NONFINITE && r.nonfinite==32 && !r.recovery_required);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
	}
	printf("Dropped every DMA post-upload control write: %u cases\n",dmaCases);
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r,false,true));
		unsigned writes=trial.Mock::writes;
		assert(c.fp16 && !PioneerRuntimeMatvecPrepare(trial,b,n,s,c,false) && trial.Mock::writes==writes);
		assert(Run(trial,h,s,c,r,false,true));
		uint32_t input[512]={},output[32]={}; input[0]=0x7c00;
		unsigned starts=trial.starts;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r) && trial.starts==starts && !r.started);
		input[0]=0x7c000000;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r) && trial.starts==starts);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
	}
	for (unsigned drop=0;drop<=runtimeWrites;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; Run(trial,h,s,c,r,false,true);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r,false,false,true));
		assert(c.int8 && !r.nonfinite && !r.recovery_required);
		unsigned writes=trial.Mock::writes;
		assert(!PioneerRuntimeMatvecPrepare(trial,b,n,s,c) && !PioneerRuntimeMatvecPrepare(trial,b,n,s,c,true,true));
		assert(trial.Mock::writes==writes && Run(trial,h,s,c,r,false,false,true));
		uint32_t input[512],output[32]={}; for (auto& value:input) value=0x40004000;
		unsigned starts=trial.starts;
		input[0]=0x40004080;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r) && !r.started && trial.starts==starts);
		input[0]=0x4000c081;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r) && trial.starts==starts);
		input[0]=0x40004000;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r,true) && trial.starts==starts);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
	}
	for (unsigned drop=0;drop<=runtimeWrites;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; Run(trial,h,s,c,r,false,false,true);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	unsigned q8Writes=0;
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r,false,false,true,true)); q8Writes=trial.Mock::writes;
		assert(c.q8 && r.staged_pages==3 && !r.nonfinite && !r.recovery_required);
		unsigned writes=trial.Mock::writes,starts=trial.starts;
		assert(!PioneerRuntimeMatvecPrepare(trial,b,n,s,c,false,true) && trial.Mock::writes==writes);
		uint32_t input[512],output[32]={},scales[32]={}; for (auto& value:input) value=0x40014001;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r));
		scales[31]=0x7f800000;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r,false,scales));
		scales[31]=0x7fc00000;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,73728,0,1024,32,input,output,r,false,scales));
		assert(trial.starts==starts && trial.Mock::writes==writes);
		assert(Run(trial,h,s,c,r,false,false,true,true)); trial.nonfinite=true;
		assert(!Run(trial,h,s,c,r,false,false,true,true) && r.state==TT_RUNTIME_NONFINITE && r.stopped && !r.recovery_required);
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
	}
	unsigned q8Cases=0;
	for (unsigned drop=0;drop<=q8Writes;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; q8Cases++; Run(trial,h,s,c,r,false,false,true,true);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	printf("Dropped every scaled-Q8 post-upload control write: %u cases\n",q8Cases);
	unsigned compactWrites=0;
	{
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s) && Run(trial,h,s,c,r,false,false,true,true,true)); compactWrites=trial.Mock::writes;
		assert(c.compact && s.l1_dirty && !s.l1_restored && r.tiles==128 && r.after[0]-r.before[0]==64);
		unsigned writes=trial.Mock::writes,starts=trial.starts;
		assert(!PioneerRuntimeMatvecPrepare(trial,b,n,s,c,false,true,true,false) && trial.Mock::writes==writes);
		uint32_t input[512],output[32]={},scales[32]={}; for (auto& value:input) value=0x40014001;
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,36864,0,1024,16,input,output,r,false,scales));
		assert(!PioneerRuntimeMatvec(trial,b,n,h,s,c,32768,0,1024,32,input,output,r,false,scales));
		assert(trial.starts==starts && trial.Mock::writes==writes);
		assert(Run(trial,h,s,c,r,false,false,true,true,true));
		assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s) && s.l1_restored);
		trial.Restored();
	}
	unsigned compactCases=0;
	for (unsigned drop=0;drop<=compactWrites;drop++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s)); if (drop<trial.Mock::writes+1026) continue;
		trial.Mock::drop=drop; compactCases++; Run(trial,h,s,c,r,false,false,true,true,true);
		if (!r.recovery_required && !c.unsafe) {
			assert(PioneerRuntimeMatvecClose(trial,b,n,s,c) && PioneerResidentLeaseEnd(trial,n,s)); trial.Restored();
		} else assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	for (unsigned fault=0;fault<7;fault++) {
		RuntimeMock trial; tt_host_page_result h; PioneerResidentLease s; PioneerRuntimeMatvecLease c;
		assert(Prepared(trial,h,s));
		trial.executeTimeout=fault==0; trial.badPc=fault==1; trial.badMathFlits=fault==2; trial.stopFails=fault==3;
		trial.failRequest=fault==4?2:0; trial.clearFails=fault==5; trial.changed=fault==6;
		assert(!Run(trial,h,s,c,r,false,false,true,true,true));
		if (fault!=6) assert(r.recovery_required && trial.quarantined);
		if (fault<3) assert(trial.stops==0);
		if (r.recovery_required) assert(!PioneerRuntimeMatvecClose(trial,b,n,s,c));
	}
	printf("Dropped every compact-Q8 post-upload control write: %u cases\n",compactCases);
}
