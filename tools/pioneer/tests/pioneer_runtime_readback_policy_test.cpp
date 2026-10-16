/* SPDX-License-Identifier: MIT */
#define PIONEER_DUAL_MOCK_ONLY
#include "pioneer_runtime_dual_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeReadback.h"

struct ReadbackMock:DualMock {
	uint32_t metadata[2][4]={},traffic[2][8]={},original[2][2048]={},returned[2][2048]={};
	unsigned writes=0,drop=0,guards=0,failGuard=0,maps=0,failMap=0,starts[2]={},stops[2]={},pcs[2]={};
	unsigned completedCores=0,failCore=0;
	unsigned badTlbSlot=~0U,badTlbWord=~0U,badMetadata=~0U,counterWord=0;
	bool outputQuarantined=false,guardFault=false,stopFault=false,timeout=false,pcFault=false,counterFault=false;
	bool lateCounter=false,lateMetadata=false,badOutputMap=false,badInputMap=false,sourceFault=false,pciFault=false;
	uint64_t outputPhysical=UINT64_C(0x190000000);
	RuntimeMock& Current() { return current?static_cast<RuntimeMock&>(second):first; }
	bool SelectRuntimeCore(unsigned core) {
		assert(core<2); selects++;
		if (selects==failSelect) { unsafe=true; return false; }
		current=core; return true;
	}
	uint32_t ResetRead() {
		guards++;
		if (guards==failGuard) guardFault=true;
		return guardFault?0:Current().reset;
	}
	uint32_t DramTlbRead(unsigned slot,unsigned word) {
		uint32_t v=current?second.DramTlbRead(slot,word):first.DramTlbRead(slot,word);
		return v^(current==failCore && slot==badTlbSlot && word==badTlbWord?1:0);
	}
	int HostPageMap(uint64_t& p,uint64_t& b) { maps++; p=first.physical+(badInputMap?4096:0); b=4096; return maps==failMap?-1:0; }
	int OutputBufferMap(uint64_t& p,uint64_t& b) { maps++; p=outputPhysical+(badOutputMap?4096:0); b=16384; return maps==failMap?-1:0; }
	int OutputBufferPageMap(unsigned page,uint64_t& p,uint64_t& b) { maps++; assert(page<4); p=outputPhysical+page*4096; b=4096; return maps==failMap?-1:0; }
	void QuarantineOutputBuffer() { outputQuarantined=true; }
	uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) {
		return DualMock::HostPciRead(p,o,s)^(pciFault && current==failCore?4:0);
	}
	uint32_t ResidentNocRead(uint32_t o) {
		for (unsigned w=0;w<4;w++) {
			if (o==0x208+w*4) return traffic[current][w];
			if (o==BlackholeHostWriteCounter(w)) return traffic[current][4+w];
		}
		uint32_t v=DualMock::ResidentNocRead(o);
		return v^(sourceFault && current==failCore && o==BlackholeDramSourceOffset(1)?1:0);
	}
	uint32_t RuntimeCodeRead(unsigned w) { return Current().code[w]; }
	void RuntimeCodeWrite(unsigned w,uint32_t value) {
		assert(!unsafe && !guardFault && Current().reset==0x47800 && w<1025);
		assert(w==1+0x508/4 || w==1+0x50c/4); // never overwrite code/payload
		if (++writes!=drop) Current().code[w]=value;
	}
	uint32_t ResidentL1Read(unsigned w) { assert(w>=0x1fc0/4 && w<=0x1fcc/4); return metadata[current][w-0x1fc0/4]; }
	void ResidentL1Write(unsigned w,uint32_t value) {
		assert(!unsafe && !guardFault && Current().reset==0x47800 && !value);
		assert(w>=0x1fc0/4 && w<=0x1fcc/4);
		if (++writes!=drop) metadata[current][w-0x1fc0/4]=value;
	}
	uint32_t RuntimePcRead() {
		pcs[current]++;
		if (pcs[current]==2) completedCores++;
		if (completedCores==2 && current==failCore && pcs[current]==3) {
			if (lateCounter) traffic[current][7]++;
			if (lateMetadata) metadata[current][3]^=1;
		}
		return pcFault && current==failCore?TT_Q8_READBACK_PARK_PC+4:TT_Q8_READBACK_PARK_PC;
	}
	void RuntimeResetWrite(uint32_t value) {
		assert(!unsafe && !guardFault && !first.quarantined && !second.quarantined);
		assert(value==0x87ffff || value==0x46800 || value==0x47800);
		if (Current().reset==0x46800 && value==0x87ffff) {
			assert(completedCores==2 && pcs[0]>=2 && pcs[1]>=2); stops[current]++;
		}
		if (++writes==drop || (stopFault && current==failCore && stops[current] && value==0x47800)) return;
		Current().reset=value;
		if (value!=0x46800) return;
		assert(first.reset==0x46800 && (current || second.reset==0x87ffff));
		starts[current]++; if (timeout && current==failCore) return;
		unsigned words=Current().code[1+0x508/4]*16; assert(words>=1024 && words<=2048);
		assert(Current().code[1+0x50c/4]==1);
		uint32_t mismatch=0;
		for (unsigned w=0;w<words;w++) mismatch|=original[current][w]^returned[current][w];
		metadata[current][0]=TT_Q8_READBACK_PHASE; metadata[current][1]=words;
		metadata[current][2]=TT_Q8_READBACK_MARKER; metadata[current][3]=mismatch;
		if (counterFault && current==failCore) traffic[current][counterWord==6?2:counterWord]++;
		if (current==failCore && badMetadata<3) metadata[current][badMetadata]^=1;
	}
	void Pause() {}
};
struct ReadbackFixture {
	ReadbackMock io;
	tt_host_page_result host[2];
	PioneerOutputBufferLease output;
	PioneerResidentLease store[2];
	PioneerRuntimeMatvecLease compute[2];
	tt_runtime_matvec_result math[2]={};
	PioneerRuntimeReadbackResult result[2];
	unsigned outputs;
	ReadbackFixture(unsigned n=64):outputs(n) {
		io.first.bufferWords=12288;
		assert(Prepared(io.first,host[0],store[0]) && SecondPrepared(io.second,host[1],store[1]));
		output.allocated=output.ready=output.memory_type_set=true; output.physical=io.outputPhysical;
		for (unsigned core=0;core<2;core++) {
			auto& c=compute[core]; c.saved=c.ready=c.bulk=true; c.buffer_words=12288;
			auto& r=math[core]; r.version=1; r.state=TT_RUNTIME_PASSED;
			r.prepared=r.context_reset=r.started=r.completed=r.stopped=1;
			r.phase=TT_RUNTIME_Q8_FINAL_PHASE; r.marker=0x4d415431; r.pc[0]=r.pc[1]=TT_RUNTIME_Q8_PARK_PC;
			r.tiles=(1024/16)*(n/32); unsigned reads=r.tiles/32;
			r.after[0]=r.after[2]=r.after[3]=reads; r.after[1]=reads*144;
			RuntimeMock& target=core?static_cast<RuntimeMock&>(io.second):io.first;
			target.code[0]=TT_Q8_READBACK_ENTRY;
			for (unsigned w=0;w<1024;w++) target.code[1+w]=BlackholeRuntimeQ8ReadbackCodeWord(w);
			target.code[1+0x508/4]=n/32; target.code[1+0x50c/4]=0;
			for (unsigned w=0;w<4;w++) io.metadata[core][w]=0xbad00000+w; // all four clears must be fresh
			for (unsigned w=0;w<2048;w++) io.original[core][w]=io.returned[core][w]=0x3e000000+w;
		}
	}
	bool Run() {
		auto first=io.first.Compute(),second=first; second.version=2;
		return PioneerRuntimeVerifyReadback(io,io.first.Baseline(),first,second,host[0],output,store,compute,math,1024,outputs,result);
	}
	void Cold() {
		assert(io.first.quarantined && io.second.quarantined && io.outputQuarantined && output.quarantined);
		for (unsigned core=0;core<2;core++) assert(result[core].cold && compute[core].unsafe && store[core].recovery_required);
	}
};
int main()
{
	(void)&Run; (void)&SecondRun; (void)&SecondClose; // shared older fixtures, not executed here
	unsigned writes=0,selects=0,guards=0,maps=0;
	for (unsigned n:{64U,2048U,4096U}) {
		ReadbackFixture f(n); assert(f.Run());
		for (unsigned core=0;core<2;core++) {
			const auto& r=f.result[core]; assert(r.armed && r.started && r.completed && r.stopped && !r.cold && !r.mismatch);
			assert(r.words==((n/2+1023)/1024)*1024 && f.io.starts[core]==1 && f.io.stops[core]==1);
			assert(f.io.first.outputPioReads==0 && f.io.second.outputPioReads==0 && !f.io.first.requests && !f.io.second.requests);
		}
		if (n==64) { writes=f.io.writes; selects=f.io.selects; guards=f.io.guards; maps=f.io.maps; }
	}
	for (unsigned drop=1;drop<=writes;drop++) {
		ReadbackFixture f; f.io.drop=drop; assert(!f.Run()); f.Cold();
	}
	for (unsigned select=1;select<=selects;select++) {
		ReadbackFixture f; f.io.failSelect=select; assert(!f.Run()); f.Cold();
	}
	for (unsigned guard=1;guard<=guards;guard++) {
		ReadbackFixture f; f.io.failGuard=guard; assert(!f.Run()); f.Cold();
	}
	for (unsigned map=1;map<=maps;map++) {
		ReadbackFixture f; f.io.failMap=map; assert(!f.Run()); f.Cold();
	}
	for (unsigned core=0;core<2;core++) for (unsigned fault=0;fault<10;fault++) {
		ReadbackFixture f; f.io.failCore=core;
		f.io.timeout=fault==0; f.io.pcFault=fault==1; f.io.counterFault=fault==2; f.io.stopFault=fault==3;
		f.io.lateCounter=fault==4; f.io.lateMetadata=fault==5;
		f.io.sourceFault=fault==6; f.io.pciFault=fault==7; f.io.badInputMap=fault==8; f.io.badOutputMap=fault==9;
		assert(!f.Run()); f.Cold();
		if (fault!=3) assert(!f.io.stops[0] && !f.io.stops[1]);
		if (fault==3 && !core) assert(!f.io.stops[1]);
	}
	for (unsigned core=0;core<2;core++) {
		for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++) {
			ReadbackFixture f; f.io.failCore=core; f.io.badTlbSlot=slot; f.io.badTlbWord=word;
			assert(!f.Run() && !f.io.writes); f.Cold();
		}
		for (unsigned word=0;word<3;word++) {
			ReadbackFixture f; f.io.failCore=core; f.io.badMetadata=word;
			assert(!f.Run() && !f.io.stops[0] && !f.io.stops[1]); f.Cold();
		}
		for (unsigned word=0;word<8;word++) {
			ReadbackFixture f; f.io.failCore=core; f.io.counterFault=true; f.io.counterWord=word;
			assert(!f.Run() && !f.io.stops[0] && !f.io.stops[1]); f.Cold();
		}
	}
	for (unsigned core=0;core<2;core++) for (unsigned word=0;word<2048;word++) {
		ReadbackFixture f(4096); f.io.returned[core][word]^=1U<<(word%32);
		assert(!f.Run() && !f.output.quarantined && f.result[core].state==TT_READBACK_MISMATCH);
		assert(f.result[0].completed && f.result[1].completed && f.result[0].stopped && f.result[1].stopped);
	}
	for (unsigned core=0;core<2;core++) for (unsigned word=0xf54/4;word<=1024;word++) {
		ReadbackFixture f; RuntimeMock& target=core?static_cast<RuntimeMock&>(f.io.second):f.io.first;
		target.code[word==1024?0:1+word]^=1; assert(!f.Run()); f.Cold();
		assert(!f.io.writes && !f.io.starts[0] && !f.io.starts[1]);
	}
	for (unsigned core=0;core<2;core++) {
		ReadbackFixture f; f.math[core].stopped=0; assert(!f.Run() && !f.io.selects && !f.io.writes);
	}
	printf("PASS: OFFLINE both-core exact comparison controller, %u lost writes/%u selections/%u reset guards/%u host maps, fresh four-word metadata, all4096 corruptions, no new NoC traffic, both stops, quarantine both host owners; NO DMA/native/output acceptance\n",writes,selects,guards,maps);
}
