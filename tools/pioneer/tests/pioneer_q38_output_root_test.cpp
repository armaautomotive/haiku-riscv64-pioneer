/* SPDX-License-Identifier: MIT */
// OFFLINE execution: explicit independent image hashes in the runner.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#include "pioneer_q38_output_fabric.h"
int main(int argc,char** argv)
{
	assert(argc==3);
	for (unsigned workers:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		OutputReturnMachine<DispatchMachine> m(argv[1],workers,type,17408);
		assert(m.RunRoot()); m.VerifyHost();
		m.Configure(2); m.dmaPackets=0; m.DmaHeader(); assert(m.RunRoot()); m.VerifyHost();
		m.Configure(3); assert(m.RunRoot() && m.dmaPackets==2*((workers+3)/4)); // CHECKED: no extra host DMA
	}
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		OutputReturnMachine<CooperativeDispatch> m(argv[1],argv[2],2,type,k);
		assert(m.RunRoot()); m.VerifyHost();
		for (auto& p:m.actual) assert(p->CorrectRows() && !p->dotProbeReads);
	}
	for (unsigned packet:{1U,30U,60U}) for (unsigned fault=0;fault<4;fault++) {
		if ((fault==1 && !(packet&1)) || (fault==2 && (packet&1))) continue;
		OutputReturnMachine<DispatchMachine> m(argv[1],119,12,5120);
		if (!fault) m.failDma=packet; else if (fault==1) m.dropDma=packet;
		else if (fault==2) m.corruptDma=packet; else m.changeSourceDma=packet;
		assert(!m.RunRoot() && m.unknown && !m.memory.at(0x7f8e0)); m.VerifyFaultInert();
	}
	for (unsigned field=9;field<16;field++) {
		OutputReturnMachine<DispatchMachine> m(argv[1],7); m.memory[0x74000+field*4]^=1;
		assert(!m.RunRoot() && !m.packets && !m.dmaPackets);
	}
	for (bool pc:{false,true}) {
		OutputReturnMachine<DispatchMachine> m(argv[1],7); if (pc) m.badPcWorker=7; else m.nonfiniteWorker=7;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	puts("PASS OFFLINE ACTUAL bounded-output root+real peers: all peer physical/source/counter/output proofs BEFORE bounded host DMA, repeated/checked mode isolation, delayed posted writes/full ordered bit comparison, every fault parks without later NoC IO;NO native hardware IO");
}
