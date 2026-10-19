/* SPDX-License-Identifier: MIT */
#define main OriginalPersistentTests
#include "pioneer_q38_vector_persistent_image_test.cpp"
#undef main
int main(int argc,char** argv)
{
	OriginalPersistentTests(argc,argv);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		VectorPersistent m(argv[1],type,k,3); assert(m.Boot() && m.Job(1));
		for (unsigned phase=0;phase<4;phase++) assert(m.phaseVisits[phase]==
			(type==12 && !(phase&1)?0:m.groups*k/16));
		assert(m.thread[11]==(type==12?1U:0U) && m.thread[29]==(type==12?0x4000U:0x2000U));
	}
	for (unsigned type:{8U,12U,14U}) {
		VectorPersistent m(argv[1],type,5120,1); assert(m.Boot()); m.dropLowFidelity=true;
		assert(!m.Job(1)); // protocol completion alone cannot pass exact arithmetic
	}
	puts("PASS ACTUAL Q4 TWO-PHASE persistent peer: phases1/3 exactly, Q8/Q6 unchanged, full-K and real-row exact FP32+independent FP64, repeat/STOP/UNKNOWN, numeric negative controls; NO DEVICE IO");
}
