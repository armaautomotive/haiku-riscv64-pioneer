/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_RUN_MOCK_ONLY
#include "pioneer_q38_dispatch_run_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
struct CloseIO:RunIO {
	unsigned closeFault=0,target=119,postPc[120]={},endHolds=0,restored=0;
	bool failedRestore=false,tlbWritten=false;
	uint32_t stopStats[64]={};
	explicit CloseIO(const char* path):RunIO(path) {}
	uint32_t RuntimePcRead() {
		if (stopExecuted) { postPc[current]++; if (current==target && closeFault==3) return 0x7204c; }
		return RunIO::RuntimePcRead();
	}
	uint32_t Q38PanelRead(uint32_t a) {
		if (current==target && a==0x72000 && (closeFault==1 || (stopExecuted && closeFault==2))) return RunIO::Q38PanelRead(a)^1;
		if (stopExecuted && current==target && closeFault==4 && a==0x74144) return RunIO::Q38PanelRead(a)+1;
		if (restored && current==target && closeFault==8 && a==0x6e000) return RunIO::Q38PanelRead(a)^1;
		return RunIO::Q38PanelRead(a);
	}
	void Q38PanelWrite(uint32_t a,uint32_t value) {
		assert(!failedRestore);
		if (stopExecuted && endHolds && a==0x6e000) {
			assert(endHolds==120); restored++;
			for (unsigned w=1;w<120;w++) assert(postPc[w]>=2 && core[w]->coreReset==0x47800);
		}
		RunIO::Q38PanelWrite(a,value);
	}
	void Execute() {
		RunIO::Execute();
		if (!stopExecuted) return;
		for (unsigned i=0;i<64;i++) stopStats[i]=C().view[(0x7f800-TT_Q38_PANEL_VIEW_BASE)/4+i];
		if (closeFault==5) C().counters[0]++;
		if (closeFault==9) core[target]->config^=1;
	}
	void RuntimeResetWrite(uint32_t value) {
		assert(!failedRestore);
		if (!stopExecuted) { RunIO::RuntimeResetWrite(value); return; }
		assert(!quarantined && !restored);
		for (unsigned w=1;w<120;w++) assert(postPc[w]>=2);
		auto& c=C(); c.resets.push_back(value);
		if (current==target && ((closeFault==6 && value==0x87ffff) || (closeFault==7 && value==0x47800))) return;
		assert(value==0x87ffff || (value==0x47800 && c.coreReset==0x87ffff));
		c.coreReset=value; if (value==0x47800) endHolds++;
	}
	void ResidentNocWrite(uint32_t a,uint32_t value) {
		assert(!failedRestore);
		if (closeFault==10 && current==target && restored && a==0x100) { RunIO::ResidentNocWrite(a,value^1); return; }
		RunIO::ResidentNocWrite(a,value);
	}
	uint32_t ResidentNocRead(uint32_t a) {
		uint32_t value=RunIO::ResidentNocRead(a);
		if (closeFault==10 && current==target && restored && a==0x100 && value==(C().source[0]^1)) failedRestore=true;
		return value;
	}
	void ResidentL1Write(unsigned w,uint32_t value) { assert(!failedRestore); RunIO::ResidentL1Write(w,value); }
	uint32_t ResidentL1Read(unsigned w) {
		if (closeFault==11 && current==target && restored && !w) { failedRestore=true; return RunIO::ResidentL1Read(w)^1; }
		return RunIO::ResidentL1Read(w);
	}
	void DramTlbWrite(unsigned s,unsigned w,uint32_t value) {
		assert(!failedRestore); RunIO::DramTlbWrite(s,w,value); if (restored && current==target) tlbWritten=true;
	}
	uint32_t DramTlbRead(unsigned s,unsigned w) {
		if (closeFault==12 && current==target && tlbWritten && !s && !w) { failedRestore=true; return RunIO::DramTlbRead(s,w)^1; }
		return RunIO::DramTlbRead(s,w);
	}
};
struct CloseFixture {
	CloseIO io; std::unique_ptr<PioneerQ38DispatchLease> lease{new PioneerQ38DispatchLease};
	std::unique_ptr<tt_q38_dispatch_probe> job{new tt_q38_dispatch_probe{}};
	std::vector<std::unique_ptr<PanelOwners>> owners;
	tt_dram_samples_result dram; PioneerResidentBanks banks; std::vector<uint8_t> initialized;
	explicit CloseFixture(const char* image,bool boot=true):io(image),initialized(PioneerResidentBanks::Pages,1) {
		dram=BankProof(*io.core[0]); assert(banks.Bind(dram));
		for (unsigned w=0;w<120;w++) {
			owners.emplace_back(new PanelOwners(*io.core[w])); auto& node=lease->worker[w];
			node.master=&owners[w]->store; node.noc=owners[w]->noc; if (w) node.noc.version=PioneerWorkerVersion(w);
			assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*lease,w));
		}
		if (!boot) return;
		assert(PioneerQ38DispatchBootstrap(io,dram,banks,*lease));
		job->version=1; job->type=12; job->inputs=5120; job->peers=7;
		for (unsigned w=0;w<7;w++) job->part[w]={w,1,64};
		for (unsigned w=0;w<2560;w++) job->activation[w]=0x40014001;
		for (unsigned b=0;b<20;b++) job->scales[b]=0x3c800000;
		assert(PioneerQ38DispatchRun(io,dram,banks,*lease,initialized.data(),unsigned(initialized.size()),*job));
	}
	bool Close() { return PioneerQ38DispatchClose(io,dram,banks,*lease,true); }
};
#ifndef PIONEER_Q38_DISPATCH_CLOSE_MOCK_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	for (unsigned slow:{1U,60U,119U}) {
		CloseFixture f(argv[1]); f.io.closing=true; f.io.stopSlowWorker=slow;
		if (!f.Close()) {
			printf("FAIL_OFFLINE_STOP_COMPLETION_ORDER slow_worker%u close_phase%u valid_stop_stats%u actual_final_route%u required_final_route%u holds%u restores%u;NO_NATIVE_CARD\n",
				slow,f.lease->closePhase,unsigned(PioneerQ38DispatchStopProof(f.io.stopStats,f.lease->authority)),
				f.io.core[0]->command[2],PioneerWorkerX(119)|(PioneerWorkerY(119)<<6),f.io.endHolds,f.io.restored);
			return 1;
		}
		assert(f.io.endHolds==120 && f.io.restored==120);
	}
	puts("PASS OFFLINE STOP completion-order regression:first/middle/last slow worker,deterministic final119 reset-status audit,unchanged exact command proof,ALL120 held/restored");
	for (bool boot:{false,true}) {
		CloseFixture f(argv[1],boot); f.io.closing=true; assert(f.Close() && f.lease->closed && !f.lease->unsafe);
		assert(!boot || (f.io.endHolds==120 && f.io.restored==120));
		unsigned writes=f.io.Writes(),selects=f.io.selects; assert(f.Close() && f.io.Writes()==writes && f.io.selects==selects);
		for (unsigned w=0;w<120;w++) {
			auto& c=*f.io.core[w];
			for (unsigned i=0;i<c.view.size();i++) assert(c.view[i]==0xab860000+i);
			assert(c.entry==0x1250006f && !f.lease->worker[w].image.dirty);
		}
		if (boot) {
			assert(PioneerQ38DispatchStopProof(f.io.stopStats,f.lease->authority));
			for (unsigned i=0;i<64;i++) { f.io.stopStats[i]^=1; assert(!PioneerQ38DispatchStopProof(f.io.stopStats,f.lease->authority)); f.io.stopStats[i]^=1; }
		}
		assert(PioneerQ38DispatchPeerLeaseEnd(f.io,f.lease->worker[0].noc,f.lease->worker[0].Store()));
		for (unsigned w=0;w<2048;w++) assert(f.io.core[0]->destination[w]==f.lease->worker[0].Store().saved[w]);
		writes=f.io.Writes(); selects=f.io.selects;
		assert(!PioneerQ38DispatchPeerLeaseEnd(f.io,f.lease->worker[0].noc,f.lease->worker[0].Store())
			&& f.io.Writes()==writes && f.io.selects==selects);
	}
	for (unsigned fault=1;fault<=12;fault++) for (unsigned target:{1U,60U,119U}) {
		CloseFixture f(argv[1]); f.io.closing=true; f.io.closeFault=fault; f.io.target=target;
		assert(!f.Close() && f.lease->unsafe && f.banks.Quarantined());
		if (fault==1 || fault==2) assert(f.lease->closeDetail[0]==0x300 && f.lease->closeDetail[1]==0x72000
			&& (f.lease->closeDetail[2]^f.lease->closeDetail[3])==1);
		if (fault==2) assert(f.lease->closePhase==52);
		if (fault==3) assert(f.lease->closePhase==53 && f.lease->closeDetail[0]==0x200
			&& f.lease->closeDetail[1]==TT_Q38_PEER_WAIT_LOAD && f.lease->closeDetail[2]==TT_Q38_PEER_WAIT_LOAD
			&& f.lease->closeDetail[3]==TT_Q38_PEER_STOP);
		if (fault==4) assert(f.lease->closePhase==53 && f.lease->closeDetail[0]==0x101
			&& f.lease->closeDetail[1]==f.lease->closeDetail[2]+1);
		if (fault==9) assert(f.lease->closePhase==51);
		if (fault<=5 || fault==9) assert(!f.io.endHolds && !f.io.restored);
		if (fault==1) assert(!f.io.stopExecuted);
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true;
		uint32_t detail[4]; memcpy(detail,f.lease->closeDetail,sizeof(detail));
		assert(!f.Close() && f.io.Writes()==writes && f.io.selects==selects);
		assert(!memcmp(detail,f.lease->closeDetail,sizeof(detail)));
		for (unsigned w=0;w<120;w++) assert(f.lease->worker[w].image.unsafe && f.lease->worker[w].Store().recovery_required);
	}
	for (unsigned fault:{1U,2U}) {
		CloseFixture f(argv[1]); f.io.closing=true; f.io.rootFault=fault;
		assert(!f.Close() && f.lease->unsafe && !f.io.restored && !f.io.endHolds);
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true;
		assert(!f.Close() && f.io.Writes()==writes && f.io.selects==selects);
	}
	for (unsigned fault:{10U,11U,12U}) {
		CloseFixture f(argv[1]); f.io.closing=true; assert(f.Close());
		f.io.closeFault=fault; f.io.target=0; f.io.tlbWritten=false;
		assert(!PioneerQ38DispatchPeerLeaseEnd(f.io,f.lease->worker[0].noc,f.lease->worker[0].Store()));
		assert(!PioneerQ38DispatchQuarantine(*f.lease,f.banks));
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true;
		assert(!f.Close() && f.io.Writes()==writes && f.io.selects==selects);
	}
	puts("PASS OFFLINE persistent END controller+actual dispatcher STOP:ALL119 terminal physical/code/source proofs before ANY reset,ALL120 held before ANY restoration,exact root2048-word status restore,64 STOP-stat faults+36 peer/3 root restoration faults+root timeout/fault quarantine ALL/no retry/reset/restore/free;NO native card/inference qualification");
}
#endif
