/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_PERSISTENT_TEST_MODEL_ONLY
#define PIONEER_Q38_TEST_INT_ACCUMULATOR
#define PIONEER_Q38_TEST_PIPELINED_MATRIX
#define PIONEER_Q38_Q4_TWO_PHASE
#include "pioneer_q38_vector_persistent_image_test.cpp"
int main(int argc,char** argv)
{
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--cached-affine"))); (void)&FreshInputs;
	bool cached=argc==4;
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		unsigned groups=k==17408?3:1;
		VectorPersistent proposed(argv[1],type,k,groups),previous(argv[2],type,k,groups);
		assert(proposed.Boot() && previous.Boot() && proposed.Job(1) && previous.Job(1));
		for (unsigned row=0;row<groups*16;row++) assert(proposed.buffers[4096+row]==previous.buffers[4096+row]);
		assert(proposed.matrixInstructions==previous.matrixInstructions && proposed.vectorOps==previous.vectorOps
			&& proposed.drains==previous.drains && proposed.requests==previous.requests && proposed.responses==previous.responses);
		unsigned panels=groups*(k/16); int reduction=int(previous.instructionBufferStores)-int(proposed.instructionBufferStores);
		if (cached) { if (type==12) assert(proposed.steps<previous.steps); }
		else assert(proposed.instructionBufferStores<previous.instructionBufferStores && reduction>=int(20*panels));
		printf("PASS %s ACTUAL type%u K%u N%u previous_instruction_buffer_stores%u proposed%u removed%d panels%u RV_steps%u_vs%u;SAME_TENSIX_OPS_NO_NATIVE_SPEED_CLAIM\n",
			cached?"CACHED AFFINE":"STATIC VECTOR",type,k,groups*16,previous.instructionBufferStores,proposed.instructionBufferStores,reduction,panels,previous.steps,proposed.steps);
		assert(proposed.Job(2,2) && previous.Job(2,2));
	}
}
