/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_PERSISTENT_TEST_MODEL_ONLY
#define PIONEER_Q38_TEST_INT_ACCUMULATOR
#define PIONEER_Q38_TEST_PIPELINED_MATRIX
#define PIONEER_Q38_Q4_TWO_PHASE
#include "pioneer_q38_vector_persistent_image_test.cpp"
int main(int argc,char** argv)
{
	assert(argc==3); (void)&FreshInputs;
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		unsigned groups=k==17408?3:1;
		VectorPersistent proposed(argv[1],type,k,groups),previous(argv[2],type,k,groups);
		assert(proposed.Boot() && previous.Boot());
		for (unsigned job=1;job<3;job++) {
			if (job==2) { proposed.NextPackets(); previous.NextPackets(); FreshInputs(proposed); FreshInputs(previous); }
			assert(proposed.Job(job) && previous.Job(job));
			for (unsigned row=0;row<groups*16;row++) assert(proposed.buffers[4096+row]==previous.buffers[4096+row]);
			unsigned panels=job*groups*k/16;
			assert(!proposed.recordRemaining && proposed.replayRecords==panels && proposed.replayExpansions==7*panels);
			assert(proposed.matrixInstructions==previous.matrixInstructions && proposed.vectorOps==previous.vectorOps
				&& proposed.drains==previous.drains && proposed.requests==previous.requests && proposed.responses==previous.responses);
		}
		assert(proposed.steps<previous.steps);
		printf("PASS OPERAND REPLAY ACTUAL type%u K%u N%u repeated changed inputs RV_steps%u_vs%u;SAME_EXPANDED_TENSIX_OPS_DRAINS_NOC_OUTPUT_BITS;NO_NATIVE_SPEED_CLAIM\n",
			type,k,groups*16,previous.steps,proposed.steps);
		assert(proposed.Job(3,2) && previous.Job(3,2) && !proposed.recordRemaining);
		VectorPersistent bad(argv[1],type,k,1); bad.dropReplayRecord=true; assert(bad.Boot());
		bool rejected=false;
		try { (void)bad.Job(1); } catch (const VectorPersistent::ReplayAccessFault&) { rejected=true; }
		assert(rejected && !bad.outputReads && !bad.matrixInstructions && !bad.memory.at(0x74144));
	}
	VectorPersistent m(argv[1],12,5120,1); assert(m.Boot());
	for (unsigned op:{0x04000000U,0x0407c020U,0x04000014U}) {
		bool rejected=false; try { m.Instruction(op); } catch (const VectorPersistent::ReplayAccessFault&) { rejected=true; }
		assert(rejected);
	}
	m.Instruction(0x04000013); // one recorded instruction,execute enabled
	bool rejected=false; try { m.Instruction(0x72070040); } catch (const VectorPersistent::ReplayAccessFault&) { rejected=true; }
	assert(rejected); // no store/address/config/control/recursive instruction in recorded decoder
	puts("PASS independent bounded REPLAY frontend: undefined/out-of-range/zero/reserved-field and non-register record reject before publication");
}
