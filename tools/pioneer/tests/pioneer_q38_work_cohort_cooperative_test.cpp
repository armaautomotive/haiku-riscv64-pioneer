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
		for (auto& p:m.actual) { assert(p->CorrectRows()); p->NextPackets(); }
		m.Configure(2); m.Bind(); m.memory[0x74020]=1; assert(m.RunRoot());
		for (auto& p:m.actual) { assert(p->CorrectRows()); p->NextPackets(); }
		m.Configure(3); m.Bind(); assert(m.RunRoot()); // unchanged CHECKED path after WORK
		for (auto& p:m.actual) assert(p->CorrectRows());
	}
	puts("PASS COOPERATIVE WORK ROOT+UNCHANGED REAL PEERS:actual emitted RV32/NoC/Tensix/SFPU instructions,full-K Q8/Q4/Q6,WORK/repeated/checked,exact FP32+independent FP64 observation;NO DEVICE IO");
}
