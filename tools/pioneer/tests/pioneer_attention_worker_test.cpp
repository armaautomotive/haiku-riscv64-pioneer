/* SPDX-License-Identifier: MIT */
// Actual dispatch bytes with a separately tested attention-entry stub.
// Not a concurrent full-decoder/native qualification.
#define PIONEER_COORDINATED_MACHINE_ONLY
#include "pioneer_coordinated_worker_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAttentionWorkerImage.h"
struct AttentionWorker:CoordinatedMachine {
	unsigned extraReads=0,extraFlits=0,extraWrites=0;
	AttentionWorker(const std::vector<uint8_t>& code,unsigned worker):CoordinatedMachine(code,1024,32,worker) { l1[Index(0x7051c)]=0; }
	uint32_t Load(uint32_t a) override {
		uint32_t value=CoordinatedMachine::Load(a);
		if (a>=0xffb20208 && a<=0xffb20214) {
			unsigned i=(a-0xffb20208)/4;
			value+=i==1?extraFlits:i==2?extraReads+extraWrites:extraReads;
		}
		return value;
	}
	void Enter() {
		assert(pc==CoordWait && l1[Index(0x71300)]==jobs && l1[Index(0x71304)]==jobs);
		l1[Index(0x7051c)]=0x31475441; l1[Index(0x71300)]=++jobs;
		UntilCoord(0x72000); assert(l1[Index(0x71304)]==jobs-1);
	}
	void CompleteStub(unsigned n) {
		assert(pc==0x72000 && n && n<=256);
		extraReads+=2*((n+63)/64); extraFlits+=8*n; extraWrites+=2;
		// The separate group-byte tests prove arithmetic/NoC/final drain.
		// This controlled stub isolates actual post-return publication.
		l1[Index(0x17ffc0)]=0x7200c; l1[Index(0x17ffc4)]=n; l1[Index(0x17ffc8)]=0x314e5441; pc=0x71200;
		UntilCoord(CoordWait);
		assert(l1[Index(0x71304)]==jobs && l1[Index(0x71360)]==0x7200c && l1[Index(0x71364)]==n
			&& l1[Index(0x71368)]==0x314e5441 && l1[Index(0x7136c)]==jobs);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2); auto code=Read(argv[1]);
	assert(code.size()==TT_ATTENTION_WORKER_IMAGE_WORDS*4);
	for (unsigned i=0;i<TT_ATTENTION_WORKER_CODE_WORDS;i++) assert(BlackholeAttentionWorkerCodeWord(i)==(i<code.size()/4?Word(code,i*4):0));
	for (unsigned w=1;w<=8;w++) {
		AttentionWorker m(code,w); m.BootCoord(); m.JobCoord(0); unsigned matrix=m.matrixInstructions;
		for (unsigned n:{1U,65U,256U}) { m.Enter(); m.CompleteStub(n); assert(m.matrixInstructions==matrix); }
		m.StopCoord(); assert(m.l1[m.Index(0x7130c)]==CoordStopped);
	}
	for (unsigned w:{9U,32U,119U}) { AttentionWorker m(code,w); m.BootCoord(); m.JobCoord(9); m.StopCoord(); }
	{ AttentionWorker m(code,1); m.BootCoord(); m.Enter(); unsigned done=m.l1[m.Index(0x71304)];
		m.failSourceRead=m.sourceReads+1; bool failed=false;
		try { m.CompleteStub(1); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.l1[m.Index(0x71304)]==done); }
	{ AttentionWorker m(code,1); m.BootCoord(); m.l1[m.Index(0x7051c)]=1; m.l1[m.Index(0x71300)]=++m.jobs; bool failed=false;
		try { m.UntilCoord(0x72000); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && !m.l1[m.Index(0x71304)] && !m.matrixInstructions); }
	puts("PASS OFFLINE actual matrix/attention dispatch,peers1..8,unchanged matrix-only peers,source proof BEFORE finished publication,live STOP,bad tag refusal;attention entry is stubbed here,NOT concurrent native qualification");
}
