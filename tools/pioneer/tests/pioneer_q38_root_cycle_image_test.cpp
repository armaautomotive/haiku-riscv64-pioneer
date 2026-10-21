/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_PREFIX
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#include "pioneer_q38_compact_output_fabric.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleData.h"
// A fabricated monotonic counter,NOT a silicon cycle/latency simulation.
// Independent observer admits ONLY CSRRS mcycle/h,x0;the common VM rejects
// all writes and other CSRs. Packet/DMA/source/PC observers remain intact.
template<class Base> struct CycleRoot:CompactReturnMachine<Base> {
	using CompactReturnMachine<Base>::CompactReturnMachine;
	std::vector<std::pair<unsigned,uint32_t>> reads;
	uint64_t epoch=UINT64_C(1)<<40;
	bool rollover=false,unstable=false,backward=false,large=false;
	unsigned lowReads=0;
	uint32_t ReadCycleCsr(unsigned csr) override {
		assert(csr==0xb00 || csr==0xb80);
		uint64_t now=epoch+uint64_t(this->steps)*19;
		if (large) now+=uint64_t(reads.size()/3)*(UINT64_C(1)<<34);
		if (backward && reads.size()>=3) now-=UINT64_C(1)<<39;
		uint32_t v=csr==0xb00?uint32_t(now):uint32_t(now>>32);
		if (rollover && reads.empty()) v--;
		if (unstable && csr==0xb80) v+=unsigned(reads.size()/2);
		if (csr==0xb00) lowReads++;
		reads.emplace_back(csr,v); return v;
	}
	void CheckCycles() const {
		std::vector<uint64_t> samples;
		assert(reads.size()%3==0 && this->memory.at(0x7f8e4)==0x31435951);
		for (unsigned i=0;i<reads.size();i+=3) {
			assert(reads[i].first==0xb80 && reads[i+1].first==0xb00 && reads[i+2].first==0xb80);
			if (reads[i].second==reads[i+2].second) samples.push_back((uint64_t(reads[i].second)<<32)|reads[i+1].second);
		}
		assert(samples.size()==4);
		for (unsigned phase=0;phase<3;phase++) {
			assert(samples[phase+1]>=samples[phase]);
			uint64_t delta=uint64_t(this->memory.at(0x7f800+(59+phase*2)*4))<<32;
			delta|=this->memory.at(0x7f800+(58+phase*2)*4);
			assert(delta==samples[phase+1]-samples[phase] && delta>0);
			if (large) assert(delta>UINT32_MAX);
		}
	}
};
int main(int argc,char** argv)
{
	assert(argc==4); setvbuf(stdout,nullptr,_IOLBF,0);
	for (auto section:{"text","output"}) {
		auto bytes=Read((std::string(argv[2])+"/"+section+".bin").c_str());
		for (unsigned w=0;w<2048;w++) assert((section==std::string("text")?PioneerQ38RootCycleWord(w)
			:PioneerQ38RootCyclePrefixWord(0x70000+w*4))==(w*4<bytes.size()?Word(bytes,w*4):0));
	}
	assert(PioneerQ38RootCycleWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38RootCyclePrefixWord(at)==UINT32_MAX);
	assert(!PioneerQ38RootCycleDataValid(nullptr) && !PioneerQ38RootCyclePhase(nullptr,0));
	for (unsigned n:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		CompactReturnMachine<DispatchMachine> old(argv[1],n,type,k);
		CycleRoot<DispatchMachine> next(argv[2],n,type,k);
		assert(old.RunRoot() && next.RunRoot()); old.VerifyCompactHost(); next.CheckCycles();
		uint32_t stats[64]={};
		for (unsigned w=0;w<64;w++) stats[w]=next.memory.at(0x7f800+w*4);
		assert(PioneerQ38RootCycleDataValid(stats) && !PioneerQ38RootCyclePhase(stats,3));
		for (unsigned phase=0;phase<3;phase++) {
			auto lo=stats[58+2*phase],hi=stats[59+2*phase]; stats[58+2*phase]=stats[59+2*phase]=0;
			assert(!PioneerQ38RootCycleDataValid(stats)); stats[58+2*phase]=lo; stats[59+2*phase]=hi;
		}
		stats[57]^=1; assert(!PioneerQ38RootCycleDataValid(stats));
		assert(old.host==next.host && old.packets==next.packets && old.dmaPackets==next.dmaPackets
			&& old.nocReads==next.nocReads && old.gathers==next.gathers && old.counts[0]==next.counts[0]);
		for (unsigned w=0;w<57;w++) assert(old.memory.at(0x7f800+w*4)==next.memory.at(0x7f800+w*4));
		unsigned reads=unsigned(next.reads.size());
		next.Configure(2); assert(next.RunRoot()); // checked:zero diagnostics/NO CSR reads
		assert(next.reads.size()==reads);
		for (unsigned w=57;w<64;w++) assert(!next.memory.at(0x7f800+w*4));
		next.active=119; next.Configure(3,true); assert(next.RunRoot());
		assert(next.reads.size()==reads);
	}
	for (bool large:{false,true}) {
		CycleRoot<DispatchMachine> m(argv[2],7,12,17408); m.rollover=true; m.large=large;
		assert(m.RunRoot()); m.CheckCycles(); assert(m.reads.size()==15); // bounded HIGH/LOW/HIGH retry
	}
	for (bool backward:{false,true}) {
		CycleRoot<DispatchMachine> m(argv[2],7); m.unstable=!backward; m.backward=backward;
		assert(!m.RunRoot() && m.unknown && !m.memory[0x7f8e4]);
		if (!backward) assert(!m.packets && m.reads.size()==12);
		m.VerifyFaultInert();
	}
	for (unsigned field=0;field<64;field++) {
		CycleRoot<DispatchMachine> m(argv[2],7); m.corruptWorker=7; m.corruptProof=field;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets && !m.memory.at(0x7f8e4)); m.VerifyFaultInert();
	}
	for (unsigned packet:{1U,20U,40U}) {
		CycleRoot<DispatchMachine> m(argv[2],7); m.faultAt=packet;
		assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet); m.VerifyFaultInert();
	}
	for (unsigned type:{8U,12U,14U}) {
		CycleRoot<CooperativeDispatch> m(argv[2],argv[3],2,type,5120);
		assert(m.RunRoot()); m.CheckCycles();
		for (auto& peer:m.actual) assert(peer->CorrectRows() && !peer->dotProbeReads);
	}
	puts("PASS OFFLINE actual ROOT cycle image:108 shape/worker cases,64-bit HIGH/LOW/HIGH and rollover/bounded instability/backward refusal,unchanged ALL57 safety fields/output/NoC/DMA,checked+STOP zero CSR reads,ALL64 peer-proof faults and NoC uncertainty inert;real peer arithmetic EXACT;FABRICATED COUNTER_NOT_NATIVE_TIME_OR_SPEED");
}
