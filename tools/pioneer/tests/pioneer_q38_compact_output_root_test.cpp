/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_PREFIX
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#include "pioneer_q38_compact_output_fabric.h"
template<class Base> struct LegacyOutputMachine:OutputReturnMachine<Base> {
	template<class... Args> LegacyOutputMachine(Args&&... args):OutputReturnMachine<Base>(std::forward<Args>(args)...) {
		this->memory[0x74024]=0x314f3851;
	}
	bool CompactOutputLayout() const override { return false; }
};
int main(int argc,char** argv)
{
	assert(argc==4); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto text=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/output.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38CompactOutputRootWord(word)==(word*4<text.size()?Word(text,word*4):0));
		assert(PioneerQ38CompactOutputRootPrefixWord(0x70000+word*4)==(word*4<prefix.size()?Word(prefix,word*4):0));
	}
	assert(PioneerQ38CompactOutputRootWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38CompactOutputRootPrefixWord(at)==UINT32_MAX);
	for (unsigned n:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		LegacyOutputMachine<DispatchMachine> old(argv[1],n,type,k);
		CompactReturnMachine<DispatchMachine> fast(argv[2],n,type,k);
		for (unsigned i=0;i<119*256;i++) fast.memory[0x80000+i*4]=0xdec00000+i;
		assert(old.RunRoot() && fast.RunRoot()); fast.VerifyCompactHost();
		unsigned packed=0;
		for (unsigned w=0;w<n;w++) for (unsigned row=0;row<fast.memory.at(0x74088+w*32)*16;row++)
			assert(old.host[w*256+row]==fast.host[packed++]);
		for (unsigned i=packed;i<119*256;i++) assert(fast.memory.at(0x80000+i*4)==0xdec00000+i);
		assert(fast.dmaPackets<=old.dmaPackets && fast.steps<old.steps);
		if (n==119 && type==12 && k==17408)
			printf("OFFLINE_COMPACT_OUTPUT rows%u bytes%u/%u DMA_packets%u/%u root_RV%u/%u;NOT_NATIVE_OR_TOKEN_RATE\n",
				packed,old.active*1024,packed*4,old.dmaPackets,fast.dmaPackets,old.steps,fast.steps);
		// Then checked and terminal STOP MUST retain original padded layout.
		fast.Configure(2); assert(fast.RunRoot());
		fast.active=119; fast.Configure(3,true); assert(fast.RunRoot());
	}
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		CompactReturnMachine<CooperativeDispatch> m(argv[2],argv[3],2,type,k);
		assert(m.RunRoot()); m.VerifyCompactHost(); for (auto& p:m.actual) assert(p->CorrectRows() && !p->dotProbeReads);
	}
	for (unsigned n:{1U,119U}) for (unsigned groups:{1U,16U}) {
		CompactReturnMachine<DispatchMachine> m(argv[2],n,12,17408);
		for (unsigned w=0;w<n;w++) m.memory[0x74088+w*32]=groups;
		assert(m.RunRoot()); m.VerifyCompactHost();
		assert(m.memory.at(0x7f8c8)==n*groups*64);
	}
	for (unsigned field=0;field<64;field++) {
		CompactReturnMachine<DispatchMachine> m(argv[2],7,12,5120); m.corruptWorker=7; m.corruptProof=field;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	for (unsigned worker:{1U,4U,7U}) for (bool pc:{false,true}) {
		CompactReturnMachine<DispatchMachine> m(argv[2],7,12,5120); if (pc) m.badPcWorker=worker; else m.nonfiniteWorker=worker;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	for (unsigned packet:{1U,20U,40U}) {
		CompactReturnMachine<DispatchMachine> m(argv[2],7,12,5120); m.faultAt=packet;
		assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet); m.VerifyFaultInert();
	}
	for (unsigned packet:{1U,4U,8U}) for (unsigned fault=0;fault<4;fault++) {
		if ((fault==1 && !(packet&1)) || (fault==2 && (packet&1))) continue;
		CompactReturnMachine<DispatchMachine> m(argv[2],119,12,5120);
		if (!fault) m.failDma=packet; else if (fault==1) m.dropDma=packet;
		else if (fault==2) m.corruptDma=packet; else m.changeSourceDma=packet;
		assert(!m.RunRoot() && m.unknown && !m.memory.at(0x7f8e0)); m.VerifyFaultInert();
	}
	for (unsigned field=9;field<16;field++) {
		CompactReturnMachine<DispatchMachine> m(argv[2],7,12,5120); m.memory[0x74000+field*4]^=1;
		assert(!m.RunRoot() && !m.packets && !m.dmaPackets);
	}
	{
		CompactReturnMachine<DispatchMachine> m(argv[2],7,12,5120); m.memory[0x74024]=0x314f3851;
		assert(!m.RunRoot() && !m.packets && !m.dmaPackets); // NEVER silently accept padded Work contract
	}
	puts("PASS OFFLINE ACTUAL compact-output root:ALL active rows EXACT vs padded root+independent real-peer math,NO padded/inactive byte transported,poisoned tails preserved,checked/STOP unchanged,all64 proof fields/PC/source/nonfinite/NoC/posted-write failures refuse publication and park without later IO;NO driver/native qualification");
}
