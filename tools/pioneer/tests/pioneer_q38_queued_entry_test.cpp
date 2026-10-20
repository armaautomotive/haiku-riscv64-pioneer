/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_UNSELECTED_QUEUED_ENTRY
#define PIONEER_Q38_VECTOR_PERSISTENT_TEST_MODEL_ONLY
#include "pioneer_q38_vector_persistent_image_test.cpp"
// Independent latency observer:hold BOTH physical ZEROACCs and SrcB's
// publication until MVMUL reaches its documented source Wait Gate. Other
// metadata/config/source accesses while these are pending are forbidden.
// This is an ordering/fault model,NOT a silicon timing/performance model.
struct QueuedEntry:VectorPersistent {
	using VectorPersistent::VectorPersistent;
	std::vector<uint32_t> clears;
	bool delayedUnpack=false,dropPublication=false;
	unsigned entries=0;
	struct EntryHazard {};
	void Instruction(uint32_t op) override {
		if (op==0x10080004 || op==0x10080005) {
			assert(clears.size()<2 && op==(clears.empty()?0x10080004U:0x10080005U));
			clears.push_back(op); return;
		}
		if (op==0x428000c1 && clears.size()==2) { assert(!delayedUnpack); delayedUnpack=true; return; }
		bool matrix=(op&~0x4000U)==0x26000020 || (op&~0x4000U)==0x26c00020;
		if (matrix && !clears.empty()) {
			assert(delayedUnpack && clears.size()==2 && !ownerB);
			if (dropPublication) throw EntryHazard{};
			for (auto clear:clears) VectorPersistent::Instruction(clear);
			VectorPersistent::Instruction(0x428000c1);
			clears.clear(); delayedUnpack=false; entries++;
			assert(ownerB); // MVMUL may pass the source gate ONLY now
		} else if (!clears.empty() && op!=0x5160000a && op!=0x5460000f) throw EntryHazard{};
		VectorPersistent::Instruction(op);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (!clears.empty() && (Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,0x4000))) throw EntryHazard{};
		VectorPersistent::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (!clears.empty() && (Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,0x4000))) throw EntryHazard{};
		return VectorPersistent::Load(a);
	}
};
int main(int argc,char** argv)
{
	assert(argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) for (bool work:{false,true}) {
		unsigned groups=k==17408?3:1;
		VectorPersistent old(argv[1],type,k,groups); QueuedEntry next(argv[2],type,k,groups);
		assert(old.Boot() && next.Boot()); old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			assert(old.Job(job) && next.Job(job));
			assert(next.entries==job*groups*k/16 && old.drains-next.drains==2*next.entries);
			assert(old.matrixInstructions==next.matrixInstructions && old.vectorOps==next.vectorOps
				&& old.requests==next.requests && old.responses==next.responses && old.dotProbeReads==next.dotProbeReads);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			assert(next.clears.empty() && !next.delayedUnpack && next.phaseQueue.empty());
		}
		old.memory[0x74038]=next.memory[0x74038]=0; // STOP never inherits a Work numeric selector
		assert(old.Job(3,2) && next.Job(3,2));
	}
	QueuedEntry bad(argv[2],12,5120,1); assert(bad.Boot()); bad.dropPublication=true; bool caught=false;
	try { (void)bad.Job(1); } catch (const QueuedEntry::EntryHazard&) { caught=true; }
	assert(caught && !bad.entries && !bad.matrixInstructions);
	puts("PASS OFFLINE actual queued entry vs original:all9 shapes/2 modes/repeated changed inputs EXACT,same physical clears/matrix/SFPU/NoC,exactly2 fewer drains/panel;delayed SrcB publication/FPU ordering,no config/Dst access before source gate;NOT native/silicon cycle qualification");
}
