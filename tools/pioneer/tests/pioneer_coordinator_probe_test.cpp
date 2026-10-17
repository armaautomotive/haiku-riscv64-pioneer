/* SPDX-License-Identifier: MIT */
#define PIONEER_PEER_PC_MOCK_ONLY
#include "pioneer_peer_pc_read_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatorProbe.h"
struct CoordinatorIO:PeerPcIO {
	std::vector<uint32_t> program=std::vector<uint32_t>(4096),backup=std::vector<uint32_t>(4096),data=std::vector<uint32_t>(8192);
	uint32_t nc[8]={100,200,300,400,500,600,300,800},status[3]={};
	unsigned starts=0,stops=0,codeWrites=0,dropCode=0,dataWrites=0,dropData=0;
	bool executeTimeout=false,badPc=false,badCount=false,badNoc=false,badMarker=false,stopFails=false;
	bool staleSummary=false,badSummary=false,badResult=false,changedPeer=false,badLiveMap=false,restoreFails=false;
	CoordinatorIO() { for (unsigned w=0;w<4096;w++) program[w]=0xdada0000+w; }
	bool CoordinatorMappingStable() { return current==0 && !(badLiveMap && starts); }
	uint32_t* CoordinatorSavedWords() { return backup.data(); }
	uint32_t CoordinatorRead(unsigned w) { assert(w<4096); return program[w]; }
	void CoordinatorWrite(unsigned w,uint32_t value) { assert(!quarantined && !current && C().reset==0x47800 && w<4096); if (++codeWrites!=dropCode && !(restoreFails && value==0xdada0000+w)) program[w]=value; }
	uint32_t BulkL1Read(unsigned w) { assert(w<8192); return data[w]; }
	void BulkL1Write(unsigned w,uint32_t value) { assert(w<8192 && !quarantined && !current && C().reset==0x47800); if (++dataWrites!=dropData) data[w]=value; }
	uint32_t RuntimeStatusRead(unsigned w) { assert(!current && w<3); return status[w]; }
	void RuntimeStatusWrite(unsigned w,uint32_t value) { assert(!current && w<3 && C().reset==0x47800); status[w]=value; }
	uint32_t RuntimePcRead() {
		if (current) return 0x70000+current*4+(changedPeer && starts?4:0);
		assert(C().reset==0x46800); return badPc?0x6c000:TT_COORDINATOR_PARK;
	}
	uint32_t ResidentNocRead(unsigned off) {
		const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
		for (unsigned i=0;i<8;i++) if (off==offsets[i]) return nc[i];
		return PeerPcIO::ResidentNocRead(off);
	}
	void RuntimeResetWrite(uint32_t value) {
		assert(!current && !quarantined && (value==0x87ffff || value==0x46800 || value==0x47800));
		if (C().reset==0x46800 && value==0x87ffff) {
			assert(!executeTimeout && !badPc && !badCount && !badNoc && !badMarker && !badLiveMap); stops++;
		}
		if (stopFails && starts && value==0x47800) return;
		C().reset=value;
		if (value!=0x46800) return;
		starts++; if (executeTimeout) return;
		assert(C().code[0]==TT_COORDINATOR_BOOT && data[0]==0x31445243 && data[1]==0 && data[2] && data[2]<=119 && data[6]);
		for (unsigned w=0;w<2048;w++) assert(program[w]==BlackholeCoordinatorCodeWord(w));
		unsigned workers=data[2];
		for (unsigned w=0;w<workers;w++) data[1024+w]=data[64+w]+(badResult?4:0);
		const unsigned delta[]={4*workers,4*workers,4*workers,4*workers,0,0,4*workers,0};
		data[3072]=workers*4; data[3073]=workers*4; data[3074]=data[3075]=0;
		data[3076]=workers; data[3077]=0; data[3078]=data[6]-(staleSummary?1:0); data[3079]=0;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++;
		if (badSummary) data[3088]++;
		program[2048]=0x12345678; // executing stack must be restored, too
		status[0]=TT_COORDINATOR_DONE; status[1]=workers+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct CoordinatorFixture {
	CoordinatorIO io; std::vector<PioneerFleetWorker> fleet=std::vector<PioneerFleetWorker>(120);
	PioneerBulkUploadLease bulk; PioneerCoordinatorLease lease; tt_coordinator_probe_result result={};
	CoordinatorFixture() {
		for (unsigned w=0;w<120;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w]; tt_host_page_result host={};
			assert(Prepared(c,host,f.Store())); f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			f.Compute().ready=true; f.preludeDirty=true; c.code[0]=0x0006b06f;
		}
		bulk.saved=bulk.ready=true;
	}
	bool Run(unsigned workers=119) {
		bool passed=PioneerCoordinatorPrepare(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),lease)
			&& PioneerCoordinatorProbe(io,io.core[0]->Baseline(),fleet.data(),bulk,lease,false,workers,result);
		if (passed) assert(PioneerCoordinatorProbeProof(result,workers,lease.nonce) && io.core[0]->code[0]==0x0006b06f);
		return passed;
	}
	bool Close() { return PioneerCoordinatorClose(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),lease,!io.quarantined); }
};
#ifndef PIONEER_COORDINATOR_PROBE_MOCK_ONLY
int main()
{
	{
		CoordinatorFixture f;
		for (unsigned workers:{1U,2U,119U,1U}) { assert(f.Run(workers)); assert(f.result.nonce==f.io.starts && f.io.stops==f.io.starts); }
		for (unsigned word=0;word<sizeof(f.result)/4;word++) { auto r=f.result; reinterpret_cast<uint32_t*>(&r)[word]^=1; assert(!PioneerCoordinatorProbeProof(r,1,f.lease.nonce)); }
		assert(f.Close()); for (unsigned w=0;w<4096;w++) assert(f.io.program[w]==0xdada0000+w);
	}
	for (unsigned fault=0;fault<12;fault++) {
		CoordinatorFixture f; f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badResult=fault==8; f.io.changedPeer=fault==9; f.io.badLiveMap=fault==10; f.io.restoreFails=fault==11;
		if (fault==11) { assert(f.Run() && !f.Close() && f.io.quarantined); continue; }
		assert(!f.Run());
		if (fault==6 || fault==7) assert(!f.io.quarantined && f.Close());
		else { assert(f.io.quarantined && f.result.recovery_required); unsigned writes=f.io.codeWrites; assert(!f.Close() && f.io.codeWrites==writes); }
		if (fault<5 || fault==10) assert(!f.io.stops);
	}
	for (unsigned word:{1U,1024U,2048U}) { CoordinatorFixture f; f.io.dropCode=word; assert(!f.Run() && !f.io.starts && f.Close()); }
	for (unsigned word:{1U,2U,3U,7U,65U,183U}) {
		CoordinatorFixture f; for (auto& value:f.io.data) value=0xdeadbeef; f.io.dropData=word;
		assert(!f.Run() && !f.io.starts && f.Close());
	}
	puts("PASS OFFLINE coordinator probe ownership,code/stack restoration,fresh nonce/full summary/NoC/PC proof,changed peer/dropped stores/timeout faults; no reset/restore after unknown completion");
}
#endif
