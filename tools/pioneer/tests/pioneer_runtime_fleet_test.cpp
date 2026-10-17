/* SPDX-License-Identifier: MIT */
#define PIONEER_RUNTIME_POLICY_MOCK_ONLY
#include "pioneer_runtime_matvec_policy_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeFleet.h"
#include <memory>
#include <vector>
struct FleetIO {
	std::vector<std::unique_ptr<RuntimeMock>> core;
	std::vector<std::vector<uint32_t>> prelude,saved;
	unsigned current=0,selects=0,failSelect=0,allStarts=0,pcReads[120]={};
	unsigned sourceSamples=0,failSource=0,pciReads=0,failPci=0;
	bool quarantined=false;
	FleetIO(unsigned n):prelude(n,std::vector<uint32_t>(1024)),saved(n,std::vector<uint32_t>(1024)) {
		for (unsigned w=0;w<n;w++) {
			core.emplace_back(new RuntimeMock); core[w]->bufferWords=12288;
			for (unsigned i=0;i<1024;i++) prelude[w][i]=0xdada0000+i;
		}
	}
	bool SelectRuntimeCore(unsigned w) { assert(w<core.size()); if (++selects==failSelect) return false; current=w; return true; }
	RuntimeMock& C() { return *core[current]; }
#define FLEET_READ(type,name,args,call) type name args { return C().name call; }
#define FLEET_WRITE(name,args,call) void name args { C().name call; }
	uint32_t SourceRead(unsigned w) { if (!w) sourceSamples++; return C().SourceRead(w)^((!w && sourceSamples==failSource)?1:0); }
	uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) { pciReads++; return failPci && pciReads>=failPci?UINT32_MAX:C().HostPciRead(p,o,s); }
	FLEET_READ(uint32_t,HostPageRead,(unsigned w),(w))
	FLEET_WRITE(HostPageWrite,(unsigned w,uint32_t v),(w,v))
	FLEET_READ(int32_t,HostPageMap,(uint64_t& p,uint64_t& b),(p,b))
	FLEET_READ(uint32_t,ResetRead,(),())
	FLEET_READ(uint32_t,RuntimeCodeRead,(unsigned w),(w))
	FLEET_WRITE(RuntimeCodeWrite,(unsigned w,uint32_t v),(w,v))
	FLEET_READ(uint32_t,RuntimeStatusRead,(unsigned w),(w))
	FLEET_WRITE(RuntimeStatusWrite,(unsigned w,uint32_t v),(w,v))
	uint32_t ResidentNocRead(uint32_t o) {
		uint32_t v=C().ResidentNocRead(o);
		if (o==20 && v==129) v=PioneerWorkerX(current)|(PioneerWorkerY(current)<<6);
		return v;
	}
	void ResidentNocWrite(uint32_t o,uint32_t v) {
		assert(!quarantined);
		if (o==0x40) assert(!current && !allStarts); // only master stages input
		if (o==20 && v) { assert(v==(PioneerWorkerX(current)|(PioneerWorkerY(current)<<6))); v=129; }
		C().ResidentNocWrite(o,v);
	}
	void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t v) {
		if (v==0x406) assert(!allStarts && !current && !quarantined);
		C().HostPciWrite(p,o,s,v);
		for (auto& c:core) c->pciCommand[p]=C().pciCommand[p];
	}
	uint32_t* RuntimeStageWords() { return core[0]->stage; }
	void RuntimeProfileMark(unsigned) {}
	uint32_t RuntimePcRead() { pcReads[current]++; return C().RuntimePcRead(); }
	uint32_t RuntimeBufferRead(unsigned w) {
		assert(!quarantined && allStarts==core.size());
		for (unsigned i=0;i<core.size();i++) assert(pcReads[i]>=2 && !core[i]->executeTimeout && !core[i]->badPc);
		return C().RuntimeBufferRead(w);
	}
	void RuntimeResetWrite(uint32_t v) {
		assert(!quarantined);
		if (v==0x46800 && !allStarts) for (auto& c:core) assert(c->reset==0x87ffff);
		if (v==0x87ffff && allStarts) {
			assert(allStarts==core.size()); for (unsigned w=0;w<core.size();w++) assert(pcReads[w]>=2);
		}
		C().RuntimeResetWrite(v);
		if (v==0x46800) {
			allStarts++;
			if (current && !C().executeTimeout) { C().counters[0]++; C().counters[1]+=256; C().accepted++; C().counters[3]++; }
		}
	}
	void Pause() {}
	void QuarantineHostPage() { quarantined=true; for (auto& c:core) c->QuarantineHostPage(); }
	uint32_t* FleetSavedWords() { return saved[current].data(); }
	uint32_t FleetCodeRead(unsigned w) { return prelude[current][w]; }
	void FleetCodeWrite(unsigned w,uint32_t v) { assert(!quarantined && C().reset==0x47800); prelude[current][w]=v; }
#undef FLEET_READ
#undef FLEET_WRITE
};
struct FleetFixture {
	FleetIO io;
	std::vector<PioneerFleetWorker> fleet;
	tt_host_page_result host={}; tt_runtime_fleet_result result={};
	unsigned n;
	FleetFixture(unsigned workers):io(workers),fleet(workers),n(workers*32) {
		for (unsigned w=0;w<workers;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w]; tt_host_page_result h={};
			assert(Prepared(c,h,f.Store())); host=h; f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			assert(PioneerRuntimeMatvecPrepare(c,c.Baseline(),f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true));
			io.current=w; assert(PioneerFleetPreludePrepare(io,f));
		}
		io.current=0;
	}
	bool Run() {
		uint32_t input[512],scale[32]; std::vector<uint32_t> output(n,0);
		for (auto& v:input) v=0x40014001;
		for (auto& v:scale) v=0x3c800000;
		bool okay=PioneerFleetMatvec(io,io.core[0]->Baseline(),host,fleet.data(),PioneerQ8CompactWeightBytes(1024,n),0,
			1024,n,unsigned(fleet.size()),input,scale,output.data(),result);
		if (okay) { assert(PioneerFleetProof(result,1024,n,unsigned(fleet.size()))); for (auto v:output) assert(v==0x40000000); }
		if (io.quarantined) for (unsigned w=0;w<fleet.size();w++) assert(fleet[w].Compute().unsafe && fleet[w].Store().recovery_required && result.core[w].recovery_required);
		return okay;
	}
	void Close() {
		for (unsigned w=0;w<fleet.size();w++) {
			io.current=w; auto& f=fleet[w]; auto& c=*io.core[w];
			if (io.quarantined) { assert(!PioneerFleetPreludeClose(io,f,false)); assert(!PioneerRuntimeMatvecClose(c,c.Baseline(),f.noc,f.Store(),f.Compute())); continue; }
			assert(PioneerFleetPreludeClose(io,f,true));
			for (unsigned i=0;i<1024;i++) assert(io.prelude[w][i]==0xdada0000+i);
			assert(PioneerRuntimeMatvecClose(c,c.Baseline(),f.noc,f.Store(),f.Compute()) && PioneerResidentLeaseEnd(c,f.noc,f.Store())); c.Restored();
		}
	}
};
#ifndef PIONEER_FLEET_POLICY_MOCK_ONLY
int main() {
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned n:{32U,64U,1024U,2048U,3072U,3840U,4096U,6144U,151936U,262144U}) {
		PioneerFleetPlan p; assert(PioneerFleetPartition(k,n,120,p)); unsigned rows=0;
		for (unsigned w=0;w<p.workers;w++) { assert(p.first[w]==rows && p.rows[w] && !(p.rows[w]&31) && p.rows[w]<=4096); rows+=p.rows[w]; }
		assert(rows==n);
	}
	for (unsigned w:{0U,121U,~0U}) { PioneerFleetPlan p; assert(!PioneerFleetPartition(1024,3840,w,p)); }
	for (unsigned n:{0U,16U,33U,262176U}) { PioneerFleetPlan p; assert(!PioneerFleetPartition(1024,n,120,p)); }
	FleetFixture good(120); assert(good.Run());
	for (unsigned w=0;w<120;w++) for (unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U}) {
		auto corrupt=good.result; ((uint32_t*)&corrupt.core[w])[field]^=1; assert(!PioneerFleetProof(corrupt,1024,3840,120));
	}
	good.Close();
	unsigned selects=0,samples=0,pci=0;
	{ FleetFixture f(4); assert(f.Run()); selects=f.io.selects; samples=f.io.sourceSamples; pci=f.io.pciReads; f.Close(); }
	for (unsigned select=1;select<=selects;select++) { FleetFixture f(4); f.io.failSelect=select; assert(!f.Run()); f.Close(); }
	for (unsigned sample=1;sample<=samples;sample++) { FleetFixture f(4); f.io.failSource=sample; assert(!f.Run()); f.Close(); }
	for (unsigned read=1;read<=pci;read++) { FleetFixture f(4); f.io.failPci=read; assert(!f.Run()); f.Close(); }
	for (unsigned w=0;w<120;w++) for (unsigned fault=0;fault<4;fault++) {
		FleetFixture f(120); auto& c=*f.io.core[w]; c.executeTimeout=fault==0; c.badPc=fault==1; c.badMathFlits=fault==2; c.stopFails=fault==3;
		assert(!f.Run());
		if (fault<3) for (auto& peer:f.io.core) assert(!peer->stops);
		f.Close();
	}
	{ FleetFixture f(4); f.io.core[3]->nonfinite=true; assert(!f.Run() && !f.io.quarantined && f.result.stopped==4); f.Close(); }
	puts("PASS offline120-worker coverage,independent counter/PC proofs,all worker faults,all4-worker select failures,global quarantine/no peer reset,no host DMA after launch");
}
#endif
