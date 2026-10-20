/* SPDX-License-Identifier: MIT */
// Both roots execute emitted RV32 instructions. This is NOT native timing.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_PREFIX
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#include "pioneer_q38_output_fabric.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SparseExportRootImage.h"

struct ExportMachine:OutputReturnMachine<DispatchMachine> {
	unsigned proofStores=0,outputStores=0;
	ExportMachine(const char* root,unsigned workers,unsigned type,unsigned inputs)
		:OutputReturnMachine<DispatchMachine>(root,workers,type,inputs) {}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x76000,119*256)) proofStores++;
		if (Range(a,0x80000,119*1024)) outputStores++;
		OutputReturnMachine<DispatchMachine>::Store(a,value);
	}
};

int main(int argc,char** argv)
{
	assert(argc==4);
	const auto text=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/output.bin").c_str());
	for (unsigned w=0;w<2048;w++) {
		assert(PioneerQ38SparseExportRootWord(w)==(w*4<text.size()?Word(text,w*4):0));
		assert(PioneerQ38SparseExportRootPrefixWord(0x70000+w*4)==(w*4<prefix.size()?Word(prefix,w*4):0));
	}
	assert(PioneerQ38SparseExportRootWord(2048)==UINT32_MAX);
	assert(PioneerQ38SparseExportRootPrefixWord(0x6fffc)==UINT32_MAX);
	assert(PioneerQ38SparseExportRootPrefixWord(0x72000)==UINT32_MAX);
	assert(PioneerQ38SparseExportRootPrefixWord(0x70001)==UINT32_MAX);
	for (unsigned n:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		ExportMachine old(argv[1],n,type,k),fast(argv[2],n,type,k);
		// Poison retained private exports: a skipped store must not be
		// mistaken for a valid proof, and no stale byte may reach output.
		for (auto* m:{&old,&fast}) {
			for (unsigned i=0;i<119*64;i++) m->memory[0x76000+i*4]=0xdec0ad00+i;
			for (unsigned i=0;i<119*256;i++) m->memory[0x80000+i*4]=0xa1100000+i;
		}
		assert(old.RunRoot() && fast.RunRoot()); old.VerifyHost(); fast.VerifyHost();
		assert(old.host==fast.host && old.packets==fast.packets && old.dmaPackets==fast.dmaPackets);
		assert(fast.proofStores==0 && old.proofStores==119*64+n*64);
		assert(old.outputStores-fast.outputStores==n*256 && fast.outputStores==119*256);
		assert(fast.steps<old.steps);
		for (unsigned i=0;i<119*64;i++) assert(fast.memory.at(0x76000+i*4)==0xdec0ad00+i);
		for (unsigned i=0;i<64;i++) assert(old.memory.at(0x7f800+i*4)==fast.memory.at(0x7f800+i*4));
		if (n==119 && type==12 && k==17408)
			printf("OFFLINE Q4/K17408/119 root RV_steps %u -> %u; private_stores %u -> %u; NOT_NATIVE_TIME_OR_TOKEN_RATE\n",
				old.steps,fast.steps,old.proofStores+old.outputStores,fast.proofStores+fast.outputStores);
		// A second Work job and then checked mode overwrite the required
		// ranges. Checked mode retains the complete original proof export.
		for (auto* m:{&old,&fast}) { m->Configure(2); m->dmaPackets=0; m->DmaHeader(); m->proofStores=m->outputStores=0; }
		assert(old.RunRoot() && fast.RunRoot()); old.VerifyHost(); fast.VerifyHost(); assert(old.host==fast.host);
		for (auto* m:{&old,&fast}) { m->Configure(3); m->proofStores=m->outputStores=0; }
		assert(old.RunRoot() && fast.RunRoot());
		assert(old.proofStores==fast.proofStores && old.outputStores==fast.outputStores);
		for (unsigned i=0;i<119*64;i++) assert(old.memory.at(0x76000+i*4)==fast.memory.at(0x76000+i*4));
		fast.active=119; fast.Configure(4,true); fast.proofStores=fast.outputStores=0;
		assert(fast.RunRoot() && fast.proofStores==119*64 && fast.outputStores==119*256);
	}
	// Run actual persistent peers, not only the synthetic proof fabric.
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		OutputReturnMachine<CooperativeDispatch> m(argv[2],argv[3],2,type,k);
		assert(m.RunRoot()); m.VerifyHost(); for (auto& p:m.actual) assert(p->CorrectRows() && !p->dotProbeReads);
	}
	// Every gathered proof field remains mandatory despite no export copy.
	for (unsigned field=0;field<64;field++) {
		ExportMachine m(argv[2],7,12,5120); m.corruptWorker=7; m.corruptProof=field;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	for (unsigned worker:{1U,4U,7U}) for (bool pc:{false,true}) {
		ExportMachine m(argv[2],7,12,5120); if (pc) m.badPcWorker=worker; else m.nonfiniteWorker=worker;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	for (unsigned packet:{1U,20U,40U}) {
		ExportMachine m(argv[2],7,12,5120); m.faultAt=packet;
		assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet); m.VerifyFaultInert();
	}
	for (unsigned fault=0;fault<4;fault++) {
		ExportMachine m(argv[2],119,12,5120);
		if (!fault) m.failDma=30; else if (fault==1) m.dropDma=1;
		else if (fault==2) m.corruptDma=30; else m.changeSourceDma=30;
		assert(!m.RunRoot() && m.unknown && !m.memory.at(0x7f8e0)); m.VerifyFaultInert();
	}
	puts("PASS ACTUAL sparse Work root: all widths/formats/worker counts, exact full outputs/summary/NoC, poisoned retained exports, repeated/checked/STOP isolation, real peers, all64 proof faults and physical/nonfinite/transport faults;NO DEVICE IO");
}
