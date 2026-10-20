/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_PERSISTENT_TEST_MODEL_ONLY
#ifndef PIONEER_Q38_TEST_INT_ACCUMULATOR
#define PIONEER_Q38_TEST_INT_ACCUMULATOR
#endif
#ifndef PIONEER_Q38_TEST_PIPELINED_MATRIX
#define PIONEER_Q38_TEST_PIPELINED_MATRIX
#endif
#ifndef PIONEER_Q38_Q4_TWO_PHASE
#define PIONEER_Q38_Q4_TWO_PHASE
#endif
#include "pioneer_q38_vector_persistent_image_test.cpp"
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		unsigned groups=k==17408?3:1;
		VectorPersistent checked(argv[1],type,k,groups),work(argv[1],type,k,groups);
		assert(checked.Boot() && work.Boot());
		work.memory[0x74038]=1;
		for (unsigned job=1;job<3;job++) {
			if (job==2) { checked.NextPackets(); work.NextPackets(); FreshInputs(checked); FreshInputs(work); }
			assert(checked.Job(job) && work.Job(job));
			assert(checked.dotProbeReads==job*groups*k && !work.dotProbeReads);
			assert(checked.matrixInstructions==work.matrixInstructions && checked.vectorOps==work.vectorOps
				&& checked.drains==work.drains && checked.requests==work.requests && checked.responses==work.responses);
			for (unsigned row=0;row<groups*16;row++) assert(checked.buffers[4096+row]==work.buffers[4096+row]);
		}
		assert(work.steps<checked.steps);
		printf("PASS ACTUAL checked/Work type%u K%u N%u exact repeated changed inputs dot_reads%u_vs%u RV_steps%u_vs%u;SAME_MATRIX_SFPU_DRAINS_NOC_OUTPUT;NO_NATIVE_SPEED_CLAIM\n",
			type,k,groups*16,checked.dotProbeReads,work.dotProbeReads,checked.steps,work.steps);
		// A following CHECKED job must restore every numeric read, not inherit
		// the previous Work hint. STOP is always checked/private selector0.
		work.NextPackets(); work.memory[0x74038]=0;
		assert(work.Job(3) && work.dotProbeReads==groups*k && work.Job(4,2));
	}
	for (unsigned type:{8U,12U,14U}) {
		VectorPersistent checked(argv[1],type,5120,1); checked.corruptDotRange=true;
		assert(checked.Boot() && !checked.Job(1) && checked.pc==TT_Q38_PEER_FAULT_PARK && checked.dotProbeReads);
		for (int value:{262145,INT32_MAX,-INT32_MAX}) {
			VectorPersistent work(argv[1],type,5120,1); work.corruptDotRange=true; work.corruptDotValue=value;
			assert(work.Boot()); work.memory[0x74038]=1;
			assert(!work.Job(1) && !work.dotProbeReads && work.pc==TT_Q38_PEER_WAIT_LOAD && work.memory.at(0x74144)==1);
			// Independent CPU observer rejects wrong numbers. Production
			// Work deliberately does not validate them. No further job/END.
			unsigned requests=work.requests; for (unsigned i=0;i<1000;i++) work.StepBlock(); assert(work.requests==requests);
		}
	}
	for (unsigned flag:{2U,UINT32_MAX}) {
		VectorPersistent m(argv[1],12,5120,1); assert(m.Boot()); m.memory[0x74038]=flag;
		assert(!m.Job(1) && !m.requests && !m.matrixInstructions);
	}
	VectorPersistent stop(argv[1],12,5120,1); assert(stop.Boot()); stop.memory[0x74038]=1;
	assert(!stop.Job(1,2) && !stop.requests && !stop.matrixInstructions);
	puts("PASS actual Work/checked numeric-mode isolation: private selector bounds/STOP, checked dot faults park, Work numeric faults intentionally unvalidated and independent observer rejects, no duplicate IO");
}
