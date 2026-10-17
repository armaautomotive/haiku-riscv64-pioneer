/* SPDX-License-Identifier: MIT */
#define PIONEER_FLEET_POLICY_MOCK_ONLY
#include "pioneer_runtime_fleet_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimePersistentFleet.h"
struct PersistentIO:FleetIO {
	unsigned launches[120]={},resets=0,jobs=0,outputReads=0;
	unsigned dropIssue=~0U,dropStop=~0U,badStopPc=~0U,dropDescriptor=~0U;
	bool dropped=false;
	bool earlyMarker[120]={};
	PersistentIO(unsigned n):FleetIO(n) {}
	void RuntimeCodeWrite(unsigned w,uint32_t v) {
		assert(!quarantined);
		if (C().reset==0x46800) {
			assert(prelude[current][TT_PERSISTENT_MAILBOX]==prelude[current][TT_PERSISTENT_MAILBOX+1]);
			assert(w>=1+0x500/4 && w<1+0x514/4);
			if (current==dropDescriptor) { dropped=true; return; }
			C().code[w]=v; return;
		}
		C().RuntimeCodeWrite(w,v);
	}
	void RuntimeStatusWrite(unsigned w,uint32_t v) {
		assert(!quarantined && w<3 && !v);
		assert(C().reset==0x47800 || C().reset==0x46800);
		C().status[w]=v;
	}
	uint32_t RuntimeStatusRead(unsigned w) {
		if (w==2 && earlyMarker[current]) { earlyMarker[current]=false; return 0; }
		return C().RuntimeStatusRead(w);
	}
	void FleetCodeWrite(unsigned w,uint32_t v) {
		assert(!quarantined);
		if (C().reset==0x47800) { FleetIO::FleetCodeWrite(w,v); return; }
		assert(C().reset==0x46800);
		if (w==TT_PERSISTENT_MAILBOX+2) { assert(v==1); prelude[current][w]=v; return; }
		assert(w==TT_PERSISTENT_MAILBOX && v==prelude[current][w]+1);
		bool stop=prelude[current][w+2];
		if (current==(stop?dropStop:dropIssue)) return;
		prelude[current][w]=v;
		if (stop) { prelude[current][w+1]=v; prelude[current][w+3]=TT_PERSISTENT_STOPPED; return; }
		jobs++;
		if (C().executeTimeout) return;
		unsigned tiles=C().code[1+0x504/4]*C().code[1+0x508/4],reads=tiles/32,peer=current!=0;
		C().status[0]=TT_RUNTIME_Q8_FINAL_PHASE; C().status[1]=tiles; C().status[2]=0x4d415431;
		C().counters[0]+=reads+peer; C().counters[1]+=reads*144+peer*256+C().badMathFlits;
		C().accepted+=reads+peer; C().counters[3]+=reads+peer;
		for (unsigned i=0;i<C().code[1+0x508/4]*16;i++) C().buffers[4096+i]=C().nonfinite?0x7fc00000:0x40000000;
		prelude[current][w+1]=v;
	}
	void RuntimeResetWrite(uint32_t v) {
		assert(!quarantined);
		if (v==0x87ffff && C().reset==0x46800) {
			// NEVER reset even a known-stopped peer while another STOP lacks
			// proof. This assertion covers all partial-launch/stop faults.
			for (unsigned w=0;w<core.size();w++) if (launches[w]) {
				assert(prelude[w][TT_PERSISTENT_MAILBOX+3]==TT_PERSISTENT_STOPPED);
				assert(w!=badStopPc && w!=dropStop && w!=dropIssue && !core[w]->executeTimeout && !core[w]->badPc);
			}
			resets++;
		}
		if (v==0x47800 && C().stopFails && launches[current]) return;
		C().reset=v;
		if (v==0x46800) { launches[current]++; prelude[current][TT_PERSISTENT_MAILBOX+3]=TT_PERSISTENT_READY; }
	}
	uint32_t RuntimePcRead() {
		pcReads[current]++;
		if (C().badPc) return 0x70000;
		if (prelude[current][TT_PERSISTENT_MAILBOX+3]==TT_PERSISTENT_STOPPED) return current==badStopPc?0x71254:TT_PERSISTENT_STOP_PC;
		return TT_PERSISTENT_WAIT+(pcReads[current]%6)*4;
	}
	void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t v) {
		assert(!quarantined);
		if (v==0x406) for (unsigned w=0;w<core.size();w++) if (launches[w]) {
			assert(!current && prelude[w][TT_PERSISTENT_MAILBOX]==prelude[w][TT_PERSISTENT_MAILBOX+1]);
			assert(prelude[w][TT_PERSISTENT_MAILBOX+3]==TT_PERSISTENT_READY);
		}
		C().HostPciWrite(p,o,s,v);
		for (auto& c:core) c->pciCommand[p]=C().pciCommand[p];
	}
	void ResidentNocWrite(uint32_t o,uint32_t v) { assert(!quarantined && (!current || o!=0x40)); C().ResidentNocWrite(o,v); }
	uint32_t RuntimeBufferRead(unsigned w) {
		assert(!quarantined);
		for (unsigned i=0;i<core.size();i++) if (launches[i]) assert(prelude[i][TT_PERSISTENT_MAILBOX]==prelude[i][TT_PERSISTENT_MAILBOX+1]);
		outputReads++; return C().RuntimeBufferRead(w);
	}
};
struct PersistentFixture {
	PersistentIO io;
	std::vector<PioneerFleetWorker> fleet;
	tt_host_page_result host={}; tt_runtime_persistent_fleet_result result={};
	unsigned n;
	PersistentFixture(unsigned workers):io(workers),fleet(120),n(workers*32) {
		for (unsigned w=0;w<workers;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w]; tt_host_page_result h={};
			assert(Prepared(c,h,f.Store())); host=h; f.noc=c.Compute(); if (w) f.noc.version=PioneerWorkerVersion(w);
			assert(PioneerRuntimeMatvecPrepare(c,c.Baseline(),f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true));
			io.current=w; assert(PioneerFleetPreludePrepare(io,f) && PioneerPersistentPrepare(io,f));
		}
		io.current=0;
	}
	bool Run(unsigned workers=0) {
		if (!workers) workers=unsigned(io.core.size());
		uint32_t input[512],scales[32]; std::vector<uint32_t> output(n,0);
		for (auto& v:input) v=0x40014001;
		for (auto& v:scales) v=0x3c800000;
		bool okay=PioneerPersistentMatvec(io,io.core[0]->Baseline(),host,fleet.data(),PioneerQ8CompactWeightBytes(1024,n),0,
			1024,workers*32,workers,input,scales,output.data(),result);
		if (okay) { assert(PioneerPersistentProof(result,1024,workers*32,workers)); for (unsigned i=0;i<workers*32;i++) assert(output[i]==0x40000000); }
		return okay;
	}
	bool Stop() { return PioneerPersistentStop(io,io.core[0]->Baseline(),fleet.data()); }
	void Restore() {
		for (unsigned w=0;w<io.core.size();w++) {
			io.current=w; auto& f=fleet[w]; auto& c=*io.core[w];
			if (io.quarantined) { assert(!PioneerFleetPreludeClose(io,f,false)); continue; }
			assert(!f.persistentActive && !f.Compute().running);
			assert(PioneerFleetPreludeClose(io,f,true));
			assert(PioneerRuntimeMatvecClose(c,io.core[0]->Baseline(),f.noc,f.Store(),f.Compute()));
			assert(PioneerResidentLeaseEnd(c,f.noc,f.Store())); c.Restored();
		}
	}
};
int main() {
	{ PersistentFixture f(4); for (unsigned w=1;w<4;w++) {
		f.io.current=w; auto& core=*f.io.core[w]; auto& worker=f.fleet[w]; worker.persistentPrepared=false; worker.noc.version=0x100+w;
		core.code[1+(TT_RUNTIME_Q8_PARK_PC-0x70000)/4]=0x0000006f;
		assert(!PioneerPersistentPrepare(f.io,worker,true,true,true,true) && !worker.persistentPrepared);
		assert(PioneerPersistentPrepare(f.io,worker,true,true,true,true,true) && worker.coordinatedInline && worker.inlineMath && worker.gluDispatch);
		for (unsigned i=0;i<1024;i++) assert(core.code[1+i]==(i==(TT_RUNTIME_Q8_PARK_PC-0x70000)/4?TT_PERSISTENT_MATH_RETURN:BlackholeRuntimeQ8InlineCodeWord(i)));
		assert(!PioneerPersistentPrepare(f.io,worker,false,true,true,true) && !PioneerPersistentPrepare(f.io,worker,true,true,true,true,false));
	} }
	{ PersistentFixture f(120); for (unsigned t=0;t<4;t++) { assert(f.Run()); for (unsigned w=0;w<120;w++) assert(f.io.launches[w]==1 && f.result.core[w].sequence==t+1 && f.result.core[w].launched==(t==0)); }
		for (unsigned w=0;w<120;w++) for (unsigned field=0;field<sizeof(tt_persistent_core_result)/4;field++) {
			auto bad=f.result; ((uint32_t*)&bad.core[w])[field]^=1;
			if (field==9 || field==8) continue; // diagnostic staged_pages/poll count
			assert(!PioneerPersistentProof(bad,1024,3840,120));
		}
		assert(!f.io.resets && f.Stop() && f.io.resets==120); f.Restore(); }
	{ PersistentFixture f(4); assert(f.Run(2) && f.Run(4) && f.Run(2)); assert(f.io.launches[0]==1 && f.io.launches[3]==1); assert(f.Stop()); f.Restore(); }
	{ PersistentFixture f(4); assert(f.Run()); for (unsigned w=0;w<4;w++) f.io.earlyMarker[w]=true;
		assert(f.Run() && !f.io.quarantined); assert(f.Stop()); f.Restore(); }
	{ PersistentFixture f(4);
		for (unsigned w=0;w<4;w++) {
			f.io.current=w; auto& core=*f.io.core[w]; auto& worker=f.fleet[w];
			worker.persistentPrepared=false;
			core.code[1+(TT_RUNTIME_Q8_PARK_PC-0x70000)/4]=0x0000006f;
			assert(PioneerPersistentPrepare(f.io,worker,true));
			assert(!PioneerPersistentPrepare(f.io,worker,false)); // immutable image selection
		}
		assert(f.Run() && f.Run()); assert(f.Stop()); f.Restore(); }
	for (unsigned w=0;w<4;w++) for (unsigned fault=0;fault<6;fault++) {
		PersistentFixture f(4); assert(f.Run()); unsigned output=f.io.outputReads;
		if (fault==0) f.io.core[w]->executeTimeout=true;
		if (fault==1) f.io.core[w]->badPc=true;
		if (fault==2) f.io.core[w]->badMathFlits=true;
		if (fault==3) f.io.dropIssue=w;
		if (fault==4) { f.fleet[w].persistentIdle=false; }
		if (fault==5) { f.io.prelude[w][TT_PERSISTENT_MAILBOX+1]--; }
		assert(!f.Run() && f.io.quarantined && !f.io.resets && f.io.outputReads==output); f.Restore();
	}
	for (unsigned w=0;w<4;w++) for (unsigned fault=0;fault<3;fault++) {
		PersistentFixture f(4); assert(f.Run());
		if (fault==0) f.io.dropStop=w;
		if (fault==1) f.io.badStopPc=w;
		if (fault==2) f.io.core[w]->stopFails=true;
		assert(!f.Stop() && f.io.quarantined); if (fault<2) assert(!f.io.resets); f.Restore();
	}
	{ PersistentFixture f(4); assert(f.Run()); f.io.core[3]->nonfinite=true; assert(!f.Run() && !f.io.quarantined && f.result.idle==4); assert(f.Stop()); f.Restore(); }
	for (unsigned worker=0;worker<4;worker++) {
		PersistentFixture f(4); assert(f.Run()); f.io.dropDescriptor=worker;
		f.io.core[worker]->code[1+0x500/4]^=4096;
		assert(!f.Run() && f.io.dropped && !f.io.resets);
		if (!worker) { assert(!f.io.quarantined && f.Stop()); } else assert(f.io.quarantined);
		f.Restore();
	}
	unsigned selects,pci,sources;
	{ PersistentFixture f(4); assert(f.Run()); selects=f.io.selects; pci=f.io.pciReads; sources=f.io.sourceSamples; assert(f.Stop()); f.Restore(); }
	for (unsigned s=1;s<=selects;s++) { PersistentFixture f(4); f.io.failSelect=s; assert(!f.Run()); if (!f.io.quarantined) assert(f.Stop()); f.Restore(); }
	for (unsigned p=1;p<=pci;p++) { PersistentFixture f(4); f.io.failPci=p; assert(!f.Run() && f.io.quarantined); assert(!f.io.resets); f.Restore(); }
	for (unsigned s=1;s<=sources;s++) { PersistentFixture f(4); f.io.failSource=s; assert(!f.Run() && f.io.quarantined); assert(!f.io.resets); f.Restore(); }
	puts("PASS offline persistent controller,repeated jobs without resets,120 proofs,changing worker count,all partial jobs/STOPs quarantined before reset,finite output gate and clean exact restoration");
}
