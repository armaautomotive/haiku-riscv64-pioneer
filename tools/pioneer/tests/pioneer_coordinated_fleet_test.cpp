/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATOR_PROBE_MOCK_ONLY
#include "pioneer_coordinator_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatedFleet.h"
struct CoordinatedIO:CoordinatorIO {
	PioneerFleetWorker* fleet=nullptr;
	unsigned pioWords=0;
	bool nonfinite=false;
	uint32_t ResidentNocRead(unsigned offset) {
		if (offset<40 || offset==0x40) return FleetIO::ResidentNocRead(offset);
		return CoordinatorIO::ResidentNocRead(offset);
	}
	void ResidentNocWrite(unsigned offset,uint32_t value) {
		uint32_t before[]={C().counters[0],C().counters[1],C().accepted,C().counters[3]};
		FleetIO::ResidentNocWrite(offset,value);
		uint32_t after[]={C().counters[0],C().counters[1],C().accepted,C().counters[3]};
		for (unsigned i=0;i<4;i++) nc[i]+=uint32_t(after[i]-before[i]);
		nc[6]=nc[2];
	}
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { CoordinatorIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && C().reset==0x87ffff && data[0]==0x31445243 && data[1]==1);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned k=data[3],n=data[4]; PioneerFleetPlan p; assert(PioneerFleetPartition(k,n,data[2],p));
		for (unsigned w=1;w<=p.workers;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle
			&& fleet[w].persistentSequence==data[64+w-1]);
		unsigned reads=(n<=8192?11:10)*p.workers,flits=n<=8192?reads-p.workers+n/16:reads,writes=2*p.workers;
		const unsigned delta[]={reads,flits,reads+writes,reads,writes,writes,reads+writes,writes};
		data[3072]=reads; data[3073]=flits; data[3074]=data[3075]=writes; data[3076]=p.workers;
		data[3077]=n; data[3078]=data[6]-(staleSummary?1:0); data[3079]=1;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++;
		if (badSummary) data[3088]++;
		program[2048]=0x12345678;
		status[0]=TT_COORDINATOR_DONE; status[1]=p.workers+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
	bool CoordinatorPayloadRead(unsigned words,uint32_t* output) {
		assert(!current && C().reset==0x47800 && words && words<=8192 && starts==stops);
		for (unsigned i=0;i<words;i++) { output[i]=nonfinite && i+1==words?0x7fc00000:0x40000000; pioWords++; }
		return true;
	}
	bool RuntimePayloadRead(unsigned first,unsigned words,uint32_t* output) {
		assert(current && first==4096 && words && words<=4096 && starts==stops
			&& fleet[current].persistentIdle && !fleet[current].Compute().running);
		for (unsigned i=0;i<words;i++) { output[i]=nonfinite && i+1==words?0x7fc00000:0x40000000; pioWords++; }
		return true;
	}
};
struct CoordinatedFixture {
	CoordinatedIO io; std::vector<PioneerFleetWorker> fleet=std::vector<PioneerFleetWorker>(120);
	PioneerBulkUploadLease bulk; PioneerCoordinatorLease lease; tt_coordinated_fleet_result result={}; tt_host_page_result host={};
	CoordinatedFixture() {
		io.fleet=fleet.data();
		for (unsigned w=0;w<120;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w];
			assert(Prepared(c,host,f.Store())); f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			f.Compute().ready=true; f.preludeDirty=true; c.code[0]=0x0006b06f;
			if (w) f.coordinated=f.persistentPrepared=f.persistentActive=f.persistentIdle=true;
		}
		bulk.saved=bulk.ready=true;
		assert(PioneerCoordinatorPrepare(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),lease));
	}
	bool Run(unsigned k=1024,unsigned n=64,unsigned limit=119) {
		std::vector<uint32_t> input(k/2,0x40014001),scale(k/32,0x3f800000),output(n,0xabcdef00);
		bool okay=PioneerCoordinatedMatvec(io,io.core[0]->Baseline(),host,fleet.data(),bulk,lease,false,
			UINT64_C(0x80000000),0,k,n,limit,input.data(),scale.data(),output.data(),result);
		if (okay) {
			assert(PioneerCoordinatedProof(result,k,n,limit,lease.nonce)); for (auto value:output) assert(value==0x40000000);
		}
		return okay;
	}
};
int main()
{
	{
		CoordinatedFixture f;
		for (unsigned n:{32U,64U,3808U,6144U,8192U,151936U,64U}) assert(f.Run(1024,n) && f.io.starts==f.io.stops);
		for (unsigned k:{2048U,3072U,5120U,6144U}) assert(f.Run(k));
		for (unsigned w=0;w<sizeof(f.result)/4;w++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[w]^=1;
			// A larger positive poll count remains a valid observation.
			if (w!=10) assert(!PioneerCoordinatedProof(r,6144,64,119,f.lease.nonce));
		}
		assert(PioneerCoordinatorClose(f.io,f.io.core[0]->Baseline(),f.fleet[0].noc,f.fleet[0].Store(),f.lease,true));
	}
	for (unsigned fault=0;fault<10;fault++) {
		CoordinatedFixture f; f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9;
		assert(!f.Run());
		if (fault==9) assert(!f.io.quarantined && f.io.stops==1 && f.result.nonfinite);
		else {
			assert(f.io.quarantined && f.result.master.recovery_required && !f.io.pioWords);
			if (fault!=5) assert(!f.io.stops);
			for (auto& peer:f.fleet) assert(peer.Compute().unsafe && peer.Store().recovery_required);
		}
	}
	for (unsigned w=0;w<120;w++) {
		CoordinatedFixture f; f.fleet[w].Compute().running=true;
		assert(!PioneerCoordinatedMasterBoundary(f.io,f.io.core[0]->Baseline(),f.fleet.data(),false) && !f.io.starts);
	}
	puts("PASS OFFLINE coordinated matrix controller,119 math workers,all K,gather/full-vocabulary bounds,fresh summary,all proof-word mutations,unknown completion => no resets/readback/restore; NOT hardware speed");
}
