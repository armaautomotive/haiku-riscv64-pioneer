/* SPDX-License-Identifier: MIT */
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#define PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
#include "pioneer_q38_panel_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProbe.h"
#include <memory>
// Policy/fault fabric,NOT the arithmetic emulator. The latter independently
// executes every candidate instruction and verifies the NoC copy contents.
struct CohortIO {
	std::vector<std::unique_ptr<PanelIO>> core;
	unsigned current=0,starts=0,stops=0,scenario=0,badWorker=0,selects=0,codeReads=0;
	unsigned pcReads[120]={}; uint64_t now=0; bool quarantined=false,closing=false,fleetEnabled=false;
	CohortIO(unsigned workers) { for (unsigned w=0;w<workers;w++) core.emplace_back(new PanelIO); }
	PanelIO& C() { return *core[current]; }
	bool SelectRuntimeCore(unsigned w) {
		assert(!quarantined && w<core.size()); current=w; selects++;
		// Each synthetic core has its own fixed-window mock. Production uses
		// the privately proven worker XY,not this mock's normalized(1,2).
		return BlackholeDramConfigure(C(),0,0,1,2,false) && BlackholeDramConfigure(C(),1,0xffa00000,1,2,false);
	}
	bool MapRuntime() { return true; }
	bool MapQ38Panel() { return C().mapping; }
	uint32_t ComputeNocRead(uint32_t offset) {
		unsigned target=120;
		for (unsigned w=0;w<120;w++) {
			bool match=true; for (unsigned i=0;i<3;i++) match &= C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(w),PioneerWorkerY(w),true,i);
			if (match) target=w;
		}
		assert(target<120);
		if (offset==0x44) return 0x1c611000|BlackholeDramSourceX(C().source,PioneerWorkerX(target))|(PioneerWorkerY(target)<<6);
		if (offset==0x48) return 0x10000|target;
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w)) return C().source[w];
		if (offset==0x40 || offset==0x840 || offset==0x1040 || offset==0x1840 || (offset>=0x240 && offset<0x280)) return 0;
		assert(false); return 0;
	}
#define C_READ(type,name,args,call) type name args { return C().name call; }
#define C_WRITE(name,args,call) void name args { assert(!quarantined); C().name call; }
	C_READ(uint32_t,SourceRead,(unsigned w),(w))
	C_READ(uint32_t,HostPciRead,(unsigned p,uint16_t o,uint8_t s),(p,o,s))
	C_READ(uint32_t,DramTlbRead,(unsigned s,unsigned w),(s,w))
	C_WRITE(DramTlbWrite,(unsigned s,unsigned w,uint32_t v),(s,w,v))
	C_READ(uint32_t,ResidentL1Read,(unsigned w),(w))
	C_WRITE(ResidentL1Write,(unsigned w,uint32_t v),(w,v))
	C_READ(bool,Q38PanelMappingStable,(),())
	C_READ(uint32_t*,Q38PanelSavedWords,(),())
	C_READ(uint32_t*,HostReadBackup,(unsigned words),(words))
	C_READ(uint32_t,RuntimeCodeRead,(unsigned w),(w))
	C_WRITE(RuntimeCodeWrite,(unsigned w,uint32_t v),(w,v))
	C_READ(uint32_t,RuntimeControlRead,(unsigned w),(w))
	C_READ(uint32_t,ResetRead,(),())
	C_READ(uint32_t,RuntimeStatusRead,(unsigned w),(w))
	C_WRITE(RuntimeStatusWrite,(unsigned w,uint32_t v),(w,v))
	C_READ(uint32_t,ResidentNocRead,(uint32_t a),(a))
	C_WRITE(ResidentNocWrite,(uint32_t a,uint32_t v),(a,v))
	void QuarantineHostPage() { quarantined=true; }
	uint32_t Q38PanelRead(uint32_t a) {
		if (a>=0x70000 && a<0x74000 && PioneerQ38PanelImmutable(a)) codeReads++;
		if (starts && a>=0x84000 && a<0x88000) {
			assert(starts==core.size() && stops==core.size());
			for (unsigned w=0;w<core.size();w++) assert(pcReads[w]>=2);
		}
		return C().Q38PanelRead(a);
	}
	void Q38PanelWrite(uint32_t a,uint32_t v) {
		assert(!quarantined);
		if (!closing && ((a>=0xa0000 && a<0xab000) || (a>=0x94000 && a<0x95000))) assert(!current && !starts); // ONE host upload only
		C().Q38PanelWrite(a,v);
	}
	uint32_t RuntimePcRead() { pcReads[current]++; return scenario==3 && current==badWorker?TT_Q38_COHORT_PARK+4:TT_Q38_COHORT_PARK; }
	void RuntimeResetWrite(uint32_t value) {
		assert(!quarantined); auto& c=C(); bool fault=current==badWorker;
		if (value==0x87ffff && !starts && fault && scenario==12) return;
		if (value==0x87ffff && starts) {
			assert(starts==core.size()); for (unsigned w=0;w<core.size();w++) assert(pcReads[w]>=2);
			if (fault && scenario==14) return;
		}
		if (value==0x46800 && !starts) for (auto& node:core) assert(node->coreReset==0x87ffff);
		c.resets.push_back(value);
		if (value==0x46800) {
			starts++; if (!(fault && scenario==13)) c.coreReset=value;
			unsigned type=c.view[(0x74004-TT_Q38_PANEL_VIEW_BASE)/4],inputs=c.view[(0x74014-TT_Q38_PANEL_VIEW_BASE)/4];
			unsigned groups=c.view[(0x7401c-TT_Q38_PANEL_VIEW_BASE)/4],bytes=PioneerQ38PanelProbeBytes(type),blocks=groups*(inputs/PioneerQ38CohortBlockInputs(type));
			assert(c.command[5]==(PioneerWorkerX(current)|(PioneerWorkerY(current)<<6)));
			c.destination[2032]=TT_Q38_COHORT_DONE; c.destination[2033]=groups*(inputs/16); c.destination[2034]=TT_Q38_COHORT_MARKER;
			unsigned reads=blocks,flits=blocks*(bytes/64);
			if (current) { reads+=(inputs*2+16383)/16384+1; flits+=(inputs*2+(((inputs/PioneerQ38CohortBlockInputs(type))*4+63)&~63U))/64; }
			c.counters[0]+=reads; c.counters[1]+=flits; c.accepted+=reads; c.counters[3]+=reads;
			c.command[0]+=(blocks-1)*bytes;
			for (unsigned row=0;row<groups*16;row++) c.view[(0x84000-TT_Q38_PANEL_VIEW_BASE)/4+row]=0x3f800000+current*256+row;
			if (fault) switch(scenario) {
				case 1:c.destination[2032]=0;break; case 2:c.destination[2034]=TT_Q38_COHORT_FAULT;break;
				case 4:c.config^=1;break; case 5:c.view[(0x72000-TT_Q38_PANEL_VIEW_BASE)/4]^=1;break;
				case 6:c.command[2]^=1;break; case 7:c.counters[0]++;break; case 8:c.counters[1]++;break;
				case 9:c.writeCounters[0]++;break; case 10:c.destination[2034]^=1;break; case 11:c.destination[2033]++;break;
				case 15:core[0]->view[(0xa0000-TT_Q38_PANEL_VIEW_BASE)/4]^=1;break;
				case 16:c.view[(0x84000-TT_Q38_PANEL_VIEW_BASE)/4+groups*16-1]=0x7f800000;break;
				case 17:c.mapping=false;break; case 18:c.accepted++;break;
			}
		} else {
			c.coreReset=value;
			if (value==0x47800 && starts) stops++;
		}
	}
	void Pause() { now+=10000; }
	uint64_t Q38CohortNow() { return now; }
	unsigned Writes() const { unsigned count=0; for (const auto& c:core) count+=c->panelWrites+unsigned(c->resets.size())+c->Mock::writes; return count; }
#undef C_READ
#undef C_WRITE
};
struct CohortFixture {
	CohortIO io; std::unique_ptr<PioneerQ38CohortLease> lease;
	std::unique_ptr<tt_q38_cohort_probe> job;
	std::vector<std::unique_ptr<PanelOwners>> owners;
	tt_dram_samples_result dram; PioneerResidentBanks banks;
	std::vector<uint8_t> initialized;
	CohortFixture(unsigned workers,unsigned type=12,unsigned inputs=5120,unsigned groups=1)
		:io(workers),lease(new PioneerQ38CohortLease),job(new tt_q38_cohort_probe{}),initialized(PioneerResidentBanks::Pages,1) {
		dram=BankProof(*io.core[0]); assert(banks.Bind(dram)); lease->prepared=workers;
		job->version=TT_Q38_COHORT_ABI; job->type=type; job->inputs=inputs; job->workers=workers;
		for (unsigned w=0;w<inputs/2;w++) job->activation[w]=PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
		for (unsigned b=0;b<inputs/PioneerQ38CohortBlockInputs(type);b++) job->scales[b]=0x3c800000;
		for (unsigned w=0;w<workers;w++) {
			owners.emplace_back(new PanelOwners(*io.core[w])); auto& n=lease->worker[w];
			n.master=&owners[w]->store; n.noc=owners[w]->noc; if (w) n.noc.version=PioneerWorkerVersion(w);
			job->part[w]={w%7,groups,64+uint64_t(w/7)*33554432};
			assert(PioneerQ38CohortPrepareWorker(io,dram,banks,n,w));
		}
	}
	bool Run() { return PioneerQ38CohortRun(io,dram,banks,*lease,initialized.data(),unsigned(initialized.size()),*job); }
	void Close(bool clean) {
		if (!clean) {
			unsigned before=io.Writes(),selects=io.selects; io.quarantined=true;
			assert(!Run() && !PioneerQ38CohortClose(io,dram,banks,*lease,false));
			assert(before==io.Writes() && selects==io.selects);
			for (unsigned w=0;w<lease->prepared;w++) assert(lease->worker[w].image.unsafe && lease->worker[w].Store().recovery_required);
			for (auto& row:job->output) for (auto value:row) assert(!value);
			return;
		}
		io.closing=true; assert(PioneerQ38CohortClose(io,dram,banks,*lease,true));
		for (unsigned w=0;w<io.core.size();w++) {
			auto& c=*io.core[w]; if (!w) assert(PioneerResidentLeaseEnd(c,lease->worker[w].noc,owners[w]->store));
			assert(!c.ReleaseHostPage()); c.Safe();
			for (unsigned a=0;a<c.view.size();a++) assert(c.view[a]==0xab860000+a);
			assert(c.entry==0x1250006f);
		}
	}
};
#ifndef PIONEER_Q38_COHORT_POLICY_MOCK_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	{
		CohortIO io(120); auto dram=BankProof(*io.core[0]);
		for (unsigned w=1;w<120;w++) {
			assert(io.SelectRuntimeCore(w)); PioneerQ38CohortWorker node;
			node.noc=io.C().Compute(); node.noc.version=PioneerWorkerVersion(w);
			assert(PioneerQ38CohortPeerLeaseBegin(io,dram,w,node) && node.Store().ready && node.Store().configured);
			assert(!PioneerQ38CohortPeerLeaseBegin(io,dram,w,node)); // no second lease/retry
			assert(PioneerResidentLeaseEnd(io,node.noc,node.Store()));
		}
	}
	for (unsigned workers:{1U,2U,7U,120U}) for (unsigned type:{8U,12U,14U}) {
		CohortFixture f(workers,type,workers==7?17408:5120,workers==2?16:1);
		unsigned reads=f.io.codeReads;
		assert(f.Run() && f.job->result.started==workers && f.job->result.completed==workers && f.job->result.stopped==workers);
		assert(TT_Q38_COHORT_PER_CALL_CODE?f.io.codeReads>reads:f.io.codeReads==reads);
		for (unsigned w=0;w<workers;w++) for (unsigned row=0;row<f.job->part[w].groups*16;row++) assert(f.job->output[w][row]==0x3f800000+w*256+row);
		f.Close(true);
	}
	for (unsigned scenario=1;scenario<=18;scenario++) for (unsigned bad:{0U,2U}) {
		CohortFixture f(3); f.io.scenario=scenario; f.io.badWorker=bad;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED
		if (scenario==5) {
			//52's explicit contract detects injected random code corruption at
			//END,NOT during every job. It MUST quarantine BEFORE any restoration.
			assert(f.Run() && !f.lease->unsafe);
			unsigned writes=0,resets=0; for (auto& c:f.io.core) { writes+=c->panelWrites; resets+=unsigned(c->resets.size()); }
			assert(!PioneerQ38CohortClose(f.io,f.dram,f.banks,*f.lease,true) && f.lease->unsafe && f.banks.Quarantined());
			unsigned afterWrites=0,afterResets=0; for (auto& c:f.io.core) { afterWrites+=c->panelWrites; afterResets+=unsigned(c->resets.size()); }
			assert(writes==afterWrites && resets==afterResets); f.io.quarantined=true;
			unsigned selects=f.io.selects; assert(!f.Run() && !PioneerQ38CohortClose(f.io,f.dram,f.banks,*f.lease,false));
			assert(selects==f.io.selects); continue;
		}
#endif
		assert(!f.Run()); bool known=scenario==16;
		assert(bool(f.job->result.recovery_required)==!known && f.lease->unsafe==!known && f.banks.Quarantined()==!known);
		f.Close(known);
	}
	for (unsigned bad:{0U,60U,119U}) {
		CohortFixture f(120); f.io.scenario=8; f.io.badWorker=bad;
		assert(!f.Run() && f.job->result.completed==120 && !f.job->result.stopped); f.Close(false);
	}
	{
		CohortFixture f(3); auto writes=f.io.Writes(); f.initialized[11]=0;
		assert(!f.Run() && !f.io.starts && f.io.Writes()==writes); f.Close(true);
	}
	puts("PASS OFFLINE actual cohort policy:12 type/width/row/1..120-worker shapes,exact264KiB+entry restore,ONE input upload,ALL completion proofs before ANY stop/output,36 fault placements+120-worker first/middle/last faults;UNKNOWN quarantines ALL banks/workers with ZERO retry/reset/restore/free;NOT native/arithmetic/inference/speed");
	printf("CODE_CHECK_POLICY driver%u ABI%u per_call=%u;52 code corruption detected at END before ANY restore,not during each call\n",TT_Q38_COHORT_DRIVER,TT_Q38_COHORT_ABI,TT_Q38_COHORT_PER_CALL_CODE);
}
#endif
