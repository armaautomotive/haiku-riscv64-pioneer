/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
int main(int argc,char** argv)
{
	assert(argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		CooperativeDispatch m(argv[1],argv[2],2,type,k);
		m.memory[0x74020]=1; assert(m.RunRoot());
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
		for (auto& p:m.actual) assert(p->memory.at(0x74038)==1 && !p->dotProbeReads);
#endif
		for (auto& p:m.actual) { assert(p->CorrectRows()); p->NextPackets(); }
		m.Configure(2); m.Bind(); m.memory[0x74020]=1; assert(m.RunRoot());
		for (auto& p:m.actual) { assert(p->CorrectRows()); p->NextPackets(); }
		m.Configure(3); m.Bind(); assert(m.RunRoot()); // unchanged CHECKED path after WORK
		for (auto& p:m.actual) assert(p->CorrectRows());
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
		for (auto& p:m.actual) assert(!p->memory.at(0x74038) && p->dotProbeReads==p->groups*k);
#endif
	}
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
	{
		CooperativeDispatch checked(argv[1],argv[2],2,12,5120);
		checked.actual[1]->corruptDotRange=true;
		assert(!checked.RunRoot() && checked.unknown && !checked.gathers);
		CooperativeDispatch work(argv[1],argv[2],2,12,5120);
		work.memory[0x74020]=1; work.actual[1]->corruptDotRange=true;
		assert(work.RunRoot() && !work.actual[1]->CorrectRows() && !work.actual[1]->dotProbeReads);
		puts("PASS actual root derives numeric-only private Work selector: checked numeric failure refuses ALL output; Work deliberate numeric fault is not validated and independent CPU observer rejects it");
	}
#endif
	puts("PASS COOPERATIVE WORK ROOT+UNCHANGED REAL PEERS:actual emitted RV32/NoC/Tensix/SFPU instructions,full-K Q8/Q4/Q6,WORK/repeated/checked,exact FP32+independent FP64 observation;NO DEVICE IO");
}
