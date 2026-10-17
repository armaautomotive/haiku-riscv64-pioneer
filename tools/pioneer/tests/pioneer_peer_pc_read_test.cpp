/* SPDX-License-Identifier: MIT */
#define PIONEER_FLEET_POLICY_MOCK_ONLY
#include "pioneer_runtime_fleet_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPeerPcRead.h"
struct PeerPcIO: FleetIO {
	uint32_t bulk[8192]={},command[10]={},counter[4]={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
	unsigned requests=0,writes=0,drop=0,pcReadCount=0;
	bool timeout=false,badCounter=false,badPc=false,dropPoison=false,badMap=false,changedPc=false;
	PeerPcIO():FleetIO(120) {}
	uint32_t DramTlbRead(unsigned s,unsigned w) { return BlackholeDramFixedTlbWord(s?0xffa00000:0,PioneerWorkerX(current),PioneerWorkerY(current),false,w); }
	bool BulkMappingStable() { return !badMap && current==0; }
	uint32_t BulkL1Read(unsigned w) { assert(w==0); return bulk[w]; }
	void BulkL1Write(unsigned w,uint32_t v) { assert(w==0 && current==0 && !quarantined); if (!dropPoison) bulk[w]=v; }
	uint32_t RuntimePcRead() { assert(current && C().reset==0x47800); pcReadCount++; return 0x70000+current*4+(changedPc && pcReadCount>2?4:0); }
	uint32_t ResidentNocRead(unsigned offset) {
		if (offset<40) return command[offset/4];
		if (offset==0x40) return timeout && requests?1:0;
		if (offset>=0x208 && offset<=0x214) return counter[(offset-0x208)/4];
		return FleetIO::ResidentNocRead(offset);
	}
	void ResidentNocWrite(unsigned offset,uint32_t v) {
		assert(!current && !quarantined && C().reset==0x47800);
		if (++writes==drop) return;
		if (offset<40) { command[offset/4]=v; return; }
		assert(offset==0x40 && v==1 && command[0]==0xffb13140 && !command[1]
			&& command[3]==0x140000 && !command[4] && command[5]==129 && !command[6]
			&& !command[7] && command[8]==4 && !command[9]);
		unsigned peer=0;
		for (unsigned w=1;w<120;w++) if (command[2]==(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6))) peer=w;
		assert(peer && core[peer]->reset==0x47800); requests++;
		if (timeout) return;
		for (auto& count:counter) count++;
		if (badCounter) counter[1]++;
		bulk[0]=0x70000+peer*4+(badPc?4:0);
	}
};
struct PeerFixture {
	PeerPcIO io; std::vector<PioneerFleetWorker> fleet=std::vector<PioneerFleetWorker>(120);
	PioneerBulkUploadLease bulk; tt_peer_pc_read_result result={};
	PeerFixture() {
		for (unsigned w=0;w<120;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w]; tt_host_page_result host={};
			assert(Prepared(c,host,f.Store())); f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			f.Compute().ready=true; f.preludeDirty=true; c.code[0]=0x0006b06f;
		}
		bulk.saved=bulk.ready=true;
	}
	bool Run(unsigned peer=119) {
		bool passed=PioneerPeerPcRead(io,io.core[0]->Baseline(),fleet.data(),bulk,false,peer,result);
		for (auto& c:io.core) assert(!c->starts && !c->stops); // no RISC reset/release, ever
		return passed;
	}
};
int main()
{
	for (unsigned peer=1;peer<120;peer++) {
		PeerFixture f; assert(f.Run(peer) && f.io.requests==2 && !f.io.current && f.bulk.dirty && !f.io.quarantined);
		for (unsigned word=0;word<sizeof(f.result)/4;word++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			// Poll count is an observation, not a prescribed exact value.
			if (word!=12 && word!=13) assert(!PioneerPeerPcReadProof(r,peer));
		}
	}
	for (unsigned fault=0;fault<7;fault++) {
		PeerFixture f; f.io.timeout=fault==0; f.io.badCounter=fault==1; f.io.badPc=fault==2;
		f.io.dropPoison=fault==3; f.io.badMap=fault==4; f.io.changedPc=fault==5;
		if (fault==6) f.io.core[119]->reset=0x46800;
		assert(!f.Run());
		if (fault<2 || fault>=5) assert(f.io.quarantined && f.result.recovery_required);
		else assert(!f.io.quarantined && !f.result.recovery_required);
		if (fault==3 || fault==4 || fault==6) assert(!f.io.requests);
	}
	// A dropped command store must never issue an unverified command.
	for (unsigned drop=1;drop<=10;drop++) {
		PeerFixture f; for (auto& v:f.io.command) v=0xdeadbeef;
		f.io.drop=drop; assert(!f.Run() && !f.io.requests && !f.io.quarantined);
	}
	for (unsigned peer:{0U,120U,UINT32_MAX}) { PeerFixture f; assert(!f.Run(peer) && !f.io.requests); }
	for (unsigned w=0;w<120;w++) {
		PeerFixture f; f.fleet[w].Compute().running=true; assert(!f.Run() && !f.io.requests);
	}
	puts("PASS: fixed peer-PC NoC reads, all119 routes/wrapping proofs, dropped writes, timeout/stale/changed-PC/map/ownership faults; no reset/DMA/retry");
}
