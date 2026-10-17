/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATOR_PROBE_MOCK_ONLY
#include "pioneer_coordinator_probe_test.cpp"
#define PIONEER_OUTPUT_BUFFER_MOCK_ONLY
#include "pioneer_output_buffer_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatedFleet.h"
struct CoordinatedIO:CoordinatorIO,OutputBufferMock {
	using FleetIO::quarantined;
	using CoordinatorIO::data;
	PioneerFleetWorker* fleet=nullptr;
	unsigned pioWords=0;
	bool nonfinite=false,hostDirty=false,inputTimeout=false;
	bool fusedMode=false,omitFusedPayload=false;
	unsigned inputReads=0,outputWrites=0,outputReads=0,compareStarts=0;
	unsigned RuntimeBufferWords() { return C().RuntimeBufferWords(); }
	void BulkHostFence() { hostDirty=false; }
	void BulkHostStore(unsigned w,uint32_t value) { assert(!quarantined && allocated && !cached && w<4096); OutputBufferMock::data[w]=value; hostDirty=true; }
	uint32_t BulkHostLoad(unsigned w) { assert(!hostDirty && allocated && !cached && w<4096); return OutputBufferMock::data[w]; }
	uint32_t ResidentNocRead(unsigned offset) {
		if (offset<40 || offset==0x40) return FleetIO::ResidentNocRead(offset);
		return CoordinatorIO::ResidentNocRead(offset);
	}
	void ResidentNocWrite(unsigned offset,uint32_t value) {
		if (offset<40) { C().ResidentNocWrite(offset,value); return; }
		if (offset==0x40 && (C().command[7]==0x212 || C().command[3]==0x144000)) {
			assert(!current && value==1 && !hostDirty && C().reset==0x47800);
			const auto* c=C().command; unsigned words=c[8]/4; bool write=c[7]==0x212;
			assert(words && words<=4096 && !(words&31) && !c[6] && !c[9]
				&& C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406);
			if (write) {
				assert((c[0]==0x84000 || c[0]==0x88000) && !c[1] && c[2]==129 && c[3]==uint32_t(physical)
					&& c[4]==uint32_t(physical>>32) && c[5]==(19|(24<<6)));
				for (unsigned i=0;i<words;i++) OutputBufferMock::data[i]=C().buffers[4096+(c[0]-0x84000)/4+i];
				outputWrites++;
			} else {
				assert(c[0]==uint32_t(physical) && c[1]==uint32_t(physical>>32) && c[2]==(19|(24<<6))
					&& c[3]==0x144000 && !c[4] && c[5]==129 && !c[7]);
				for (unsigned i=0;i<words;i++) data[4096+i]=OutputBufferMock::data[i]; outputReads++;
			}
			C().requests++; C().doorbells++; C().doorbellRead=false; C().pending=false; C().done=true;
			unsigned at=write?4:0; nc[at]++; nc[at+1]+=words/16; nc[2]++; nc[6]=nc[2]; nc[at+3]++;
			return;
		}
		uint32_t before[]={C().counters[0],C().counters[1],C().accepted,C().counters[3]};
		if (offset==0x40 && C().command[3]==0x80000 && C().command[8]==16384) {
			assert(!current && value==1 && !hostDirty && allocated && !cached);
			const uint32_t expected[]={uint32_t(physical),uint32_t(physical>>32),19|(24<<6),0x80000,0,129,0,0,16384,0};
			assert(!memcmp(C().command,expected,sizeof(expected)) && C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406);
			inputReads++; C().requests++; C().doorbells++; C().doorbellRead=false;
			C().pending=inputTimeout; C().done=!inputTimeout;
			if (!inputTimeout) {
				for (unsigned i=0;i<4096;i++) C().buffers[i]=OutputBufferMock::data[i];
				C().counters[0]++; C().counters[1]+=256; C().accepted++; C().counters[3]++;
			}
		} else FleetIO::ResidentNocWrite(offset,value);
		uint32_t after[]={C().counters[0],C().counters[1],C().accepted,C().counters[3]};
		for (unsigned i=0;i<4;i++) nc[i]+=uint32_t(after[i]-before[i]);
		nc[6]=nc[2];
	}
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { CoordinatorIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && C().reset==0x87ffff && data[0]==0x31445243 && (data[1]==1 || data[1]==2 || data[1]==3));
		for (unsigned w=0;w<TT_COORDINATOR_CODE_WORDS;w++) assert(program[w]==PioneerCoordinatorWord(true,!fusedMode,w,fusedMode));
		C().reset=value; starts++; if (executeTimeout) return;
		if (data[1]==2) {
			assert(C().pciCommand[0]==0x402 && C().pciCommand[1]==0x402); compareStarts++;
			unsigned offset=data[3],words=data[4]; uint32_t mismatch=0;
			for (unsigned i=0;i<words;i++) mismatch |= C().buffers[4096+offset+i]^data[4096+i];
			data[3072]=mismatch; data[3073]=offset; data[3074]=data[3075]=0; data[3076]=data[3077]=words;
			data[3078]=data[6]; data[3079]=2;
			for (unsigned i=0;i<8;i++) data[3080+i]=data[3088+i]=nc[i];
			status[0]=TT_COORDINATOR_DONE; status[1]=words; status[2]=TT_COORDINATOR_MARKER; return;
		}
		unsigned k=data[3],n=data[4]; PioneerFleetPlan p; assert(PioneerFleetPartition(k,n,data[2],p));
		for (unsigned w=1;w<=p.workers;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle
			&& fleet[w].persistentSequence==data[64+w-1]);
		bool fused=data[1]==3;
		if (fused) {
			assert(fusedMode && n<=4096 && C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406
				&& data[7]==uint32_t(physical) && data[8]==uint32_t(physical>>32) && data[9]==16384);
			if (!omitFusedPayload) for (unsigned i=0;i<n;i++) OutputBufferMock::data[i]=data[4096+i]=0x40000000;
		}
		unsigned reads=(n<=8192?11:10)*p.workers+(fused?1:0),flits=n<=8192?reads-p.workers+n/16+(fused?n/16-1:0):reads;
		unsigned writes=2*p.workers+(fused?1:0),writeFlits=2*p.workers+(fused?n/16:0);
		const unsigned delta[]={reads,flits,reads+writes,reads,writes,writeFlits,reads+writes,writes};
		data[3072]=reads; data[3073]=flits; data[3074]=writes; data[3075]=writeFlits; data[3076]=p.workers;
		data[3077]=n; data[3078]=data[6]-(staleSummary?1:0); data[3079]=fused?3:1;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++;
		if (badSummary) data[3088]++;
		program[2048]=0x12345678;
		if (n<=8192) for (unsigned i=0;i<n;i++) C().buffers[4096+i]=0x40000000;
		status[0]=TT_COORDINATOR_DONE; status[1]=p.workers+(badCount?1:0); status[2]=(badMarker || nonfinite)?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
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
	PioneerBulkUploadLease bulk; PioneerOutputBufferLease transfer; PioneerCoordinatorLease lease; tt_coordinated_fleet_result result={}; tt_host_page_result host={};
	CoordinatedFixture(bool fused=false) {
		io.fusedMode=fused;
		io.fleet=fleet.data();
		for (unsigned w=0;w<120;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w];
			assert(Prepared(c,host,f.Store())); f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			f.Compute().ready=f.Compute().saved=f.Compute().bulk=true; f.Compute().buffer_words=TT_Q8_BULK_SAVED_BUFFER_WORDS;
			c.bufferWords=TT_Q8_BULK_SAVED_BUFFER_WORDS; f.preludeDirty=true; c.code[0]=0x0006b06f;
			if (w) f.coordinated=f.persistentPrepared=f.persistentActive=f.persistentIdle=true;
		}
		bulk.saved=bulk.ready=true;
		assert(PioneerPrepareOutputBuffer(io,host,transfer));
		assert(PioneerCoordinatorPrepare(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),lease,true,!fused,fused));
	}
	bool Run(unsigned k=1024,unsigned n=64,unsigned limit=119) {
		std::vector<uint32_t> input(k/2,0x40014001),scale(k/32,0x3f800000),output(n,0xabcdef00);
		bool okay=PioneerCoordinatedMatvec(io,io.core[0]->Baseline(),host,transfer,fleet.data(),bulk,lease,false,
			UINT64_C(0x80000000),0,k,n,limit,input.data(),scale.data(),output.data(),result);
		if (okay) {
			assert(PioneerCoordinatedProof(result,k,n,limit,lease.nonce,lease.readback)); for (auto value:output) assert(value==0x40000000);
			assert(io.inputReads==io.starts-io.compareStarts && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
		}
		return okay;
	}
};
#ifndef PIONEER_COORDINATED_FLEET_MOCK_ONLY
int main()
{
	{
		CoordinatedFixture f(true);
		for (unsigned n:{32U,64U,1024U,2048U,3072U,4096U,6144U,8192U,151936U,64U}) {
			unsigned before=f.io.pioWords; assert(f.Run(1024,n) && f.io.starts==f.io.stops && !f.transfer.pending && !f.io.compareStarts);
			if (n<=4096) assert(f.io.pioWords==before && f.result.dma_words==n);
		}
		for (unsigned k:{2048U,3072U,5120U,6144U}) assert(f.Run(k));
	}
	for (unsigned fault=0;fault<11;fault++) {
		CoordinatedFixture f(true); f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitFusedPayload=fault==10;
		assert(!f.Run() && f.io.quarantined && f.transfer.quarantined && f.result.master.recovery_required && !f.io.pioWords
			&& f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=10) assert(!f.io.stops);
	}
	{
		CoordinatedFixture f;
		for (unsigned n:{32U,64U,3808U,6144U,8192U,151936U,64U}) assert(f.Run(1024,n) && f.io.starts==f.io.stops);
		for (unsigned k:{2048U,3072U,5120U,6144U}) assert(f.Run(k));
		for (unsigned w=0;w<sizeof(f.result)/4;w++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[w]^=1;
			// A larger positive poll count remains a valid observation.
			if (w!=10) assert(!PioneerCoordinatedProof(r,6144,64,119,f.lease.nonce,true));
		}
		assert(PioneerCoordinatorClose(f.io,f.io.core[0]->Baseline(),f.fleet[0].noc,f.fleet[0].Store(),f.lease,true));
	}
	for (unsigned fault=0;fault<10;fault++) {
		CoordinatedFixture f; f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9;
		assert(!f.Run());
		{
			assert(f.io.quarantined && f.result.master.recovery_required && !f.io.pioWords);
			if (fault!=5) assert(!f.io.stops);
			for (auto& peer:f.fleet) assert(peer.Compute().unsafe && peer.Store().recovery_required);
		}
	}
	for (unsigned w=0;w<120;w++) {
		CoordinatedFixture f; f.fleet[w].Compute().running=true;
		assert(!PioneerCoordinatedMasterBoundary(f.io,f.io.core[0]->Baseline(),f.fleet.data(),false) && !f.io.starts);
	}
	{
		CoordinatedFixture f; f.io.inputTimeout=true;
		assert(!f.Run() && f.io.quarantined && f.transfer.quarantined && f.transfer.pending && !f.io.starts && !f.io.stops && !f.io.pioWords);
	}
	puts("PASS OFFLINE coordinated matrix controller,119 math workers,all K,gather/full-vocabulary bounds,fresh summary,all proof-word mutations,unknown completion => no resets/readback/restore; NOT hardware speed");
}
#endif
