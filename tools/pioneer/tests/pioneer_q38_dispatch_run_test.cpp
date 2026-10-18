/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
#include "pioneer_q38_dispatch_bootstrap_test.cpp"
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchRun.h"
#include "../PioneerQ38DispatchHost.h"
struct ControllerMachine:DispatchMachine {
	uint32_t ownSource[16];
	ControllerMachine(const char* path,unsigned n,unsigned type,unsigned k,const uint32_t* source):DispatchMachine(path,n,type,k)
	{ for (unsigned i=0;i<16;i++) ownSource[i]=source[i]; }
	uint32_t Load(uint32_t a) override {
		for (unsigned i=0;i<16;i++) if (a==0xffb20000+BlackholeDramSourceOffset(i)) return ownSource[i];
		return DispatchMachine::Load(a);
	}
};
struct RunIO:BootstrapIO {
	const char* image; unsigned rootFault=0,rootArms=0,rootStops=0,rootHolds=0,outputReads=0;
	uint32_t peerDone[119]={}; bool rootExecuted=false;
	bool peerStopped[119]={},stopExecuted=false;
	unsigned stopSlowWorker=0;
	unsigned stopSnapshotMode=0,pcSamples[120]={};
	explicit RunIO(const char* path):image(path) {}
	uint32_t RuntimePcRead() {
		bool next=stopSnapshotMode==1 || (stopSnapshotMode==2 && (pcSamples[current]++&1));
		return current?(peerStopped[current-1]?(next?TT_Q38_PEER_STOP_FETCH:TT_Q38_PEER_STOP):BootstrapIO::RuntimePcRead()):
			(next?TT_Q38_DISPATCH_PARK_FETCH:TT_Q38_DISPATCH_PARK);
	}
	uint32_t Q38PanelRead(uint32_t a) {
		if (!current && rootExecuted && a>=0x80000 && a<0x9dc00) outputReads++;
		return C().Q38PanelRead(a);
	}
	virtual void Execute() {
		auto& c=C(); const unsigned base=(0x74000-TT_Q38_PANEL_VIEW_BASE)/4;
		unsigned type=c.view[base+3],k=c.view[base+4],n=c.view[base+5];
		uint32_t source[16]; for (unsigned i=0;i<16;i++) source[i]=i?c.source[i]:c.config;
		ControllerMachine m(image,n,type,k,source);
		bool stop=c.view[base+2]==2; if (stop) m.Configure(1,true);
		m.stopSlowWorker=stopSlowWorker;
		m.stopSnapshotMode=stopSnapshotMode;
		for (unsigned word=0;word<c.view.size();word++) m.memory[TT_Q38_PANEL_VIEW_BASE+word*4]=c.view[word];
		for (unsigned i=0;i<8;i++) m.counts[i]=c.ResidentNocRead(PioneerQ38PanelCounter(i));
		for (unsigned w=0;w<119;w++) m.peers[w].requested=m.peers[w].finished=peerDone[w];
		assert(m.RunRoot());
		for (unsigned w=0;w<119;w++) {
			peerDone[w]=m.peers[w].finished;
			if (w<n) {
				peerStopped[w]=stop;
				auto& peer=*core[w+1]; unsigned at=(0x74140-TT_Q38_PANEL_VIEW_BASE)/4;
				for (unsigned i=0;i<16;i++) peer.view[at+i]=0;
				peer.view[at]=peer.view[at+1]=peerDone[w]; peer.view[at+3]=TT_Q38_PEER_MARKER; peer.view[at+4]=stop?2:0;
			}
		}
		for (const auto& value:m.memory) if ((value.first>=0x75000 && value.first<0x75800)
			|| (value.first>=0x76000 && value.first<0x7d700) || (value.first>=0x7f800 && value.first<0x7f900)
			|| (value.first>=0x80000 && value.first<0x9dc00)) c.view[(value.first-TT_Q38_PANEL_VIEW_BASE)/4]=value.second;
		c.counters[0]=m.counts[0]; c.counters[1]=m.counts[1]; c.accepted=m.counts[2]; c.counters[3]=m.counts[3];
		c.writeCounters[0]=m.counts[4]; c.writeCounters[1]=m.counts[5]; c.writeCounters[3]=m.counts[7];
		for (unsigned i=0;i<10;i++) c.command[i]=m.command[i];
		c.destination[2032]=TT_Q38_DISPATCH_DONE; c.destination[2033]=n; c.destination[2034]=TT_Q38_DISPATCH_MARKER;
		rootExecuted=true; stopExecuted=stop;
		if (rootFault==2) c.destination[2034]=TT_Q38_DISPATCH_FAULT;
		if (rootFault==3) c.view[(0x76000+(n-1)*256+32*4-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
		if (rootFault==4) c.counters[0]++;
		if (rootFault==5) c.view[(0xa0000-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
		if (rootFault==6) c.view[(0x80000+(n-1)*1024-TT_Q38_PANEL_VIEW_BASE)/4]=0x7f800000;
		if (rootFault==7) c.config^=1;
		if (rootFault==10) c.command[2]^=1;
	}
	void RuntimeResetWrite(uint32_t value) {
		if (current) { BootstrapIO::RuntimeResetWrite(value); return; }
		assert(!quarantined); auto& c=C(); c.resets.push_back(value);
		if (value==0x87ffff) {
			if (c.coreReset==0x46800) { rootStops++; assert(rootFault==0 || rootFault==8 || rootFault==9); if (rootFault==8) return; }
			else { rootArms++; rootExecuted=false; }
			c.coreReset=value; return;
		}
		if (value==0x47800) { rootHolds++; if (rootFault!=9) c.coreReset=value; return; }
		assert(value==0x46800 && c.coreReset==0x87ffff); c.coreReset=value;
		if (rootFault!=1) Execute();
	}
};
struct RunFixture {
	RunIO io; std::unique_ptr<PioneerQ38DispatchLease> lease{new PioneerQ38DispatchLease};
	std::unique_ptr<tt_q38_dispatch_probe> job{new tt_q38_dispatch_probe{}};
	std::vector<std::unique_ptr<PanelOwners>> owners;
	tt_dram_samples_result dram; PioneerResidentBanks banks; std::vector<uint8_t> initialized;
	RunFixture(const char* image,unsigned n,unsigned type=12,unsigned k=5120):io(image),initialized(PioneerResidentBanks::Pages,1) {
		dram=BankProof(*io.core[0]); assert(banks.Bind(dram));
		for (unsigned w=0;w<120;w++) {
			owners.emplace_back(new PanelOwners(*io.core[w])); auto& node=lease->worker[w];
			node.master=&owners[w]->store; node.noc=owners[w]->noc; if (w) node.noc.version=PioneerWorkerVersion(w);
			assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*lease,w));
		}
		assert(PioneerQ38DispatchBootstrap(io,dram,banks,*lease));
		job->version=1; job->type=type; job->inputs=k; job->peers=n;
		for (unsigned w=0;w<n;w++) job->part[w]={w%7,1+w%3,64+uint64_t(w/7)*0x00800000};
		for (unsigned w=0;w<k/2;w++) job->activation[w]=0x40014001;
		for (unsigned b=0;b<k/PioneerQ38CohortBlockInputs(type);b++) job->scales[b]=0x3c800000;
	}
	bool Run() { return PioneerQ38DispatchRun(io,dram,banks,*lease,initialized.data(),unsigned(initialized.size()),*job); }
};
#ifndef PIONEER_Q38_DISPATCH_RUN_MOCK_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	for (unsigned n:{1U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		RunFixture f(argv[1],n,type,type==12?17408:5120); PioneerQ38DispatchHistory history;
		assert(f.Run() && history.Accept(*f.job) && !history.Accept(*f.job) && f.Run());
		for (unsigned i=0;i<64;i++) {
			f.job->result.stats[i]^=1; assert(!history.Accept(*f.job)); f.job->result.stats[i]^=1;
		}
		for (unsigned w=0;w<n;w++) for (unsigned i=0;i<64;i++) {
			f.job->result.peer[w][i]^=1; assert(!history.Accept(*f.job)); f.job->result.peer[w][i]^=1;
		}
		assert(history.Accept(*f.job) && !history.Accept(*f.job));
		assert(f.lease->rootNonce==2 && f.io.rootArms==2 && f.io.rootStops==2 && f.io.rootHolds==2 && !f.lease->unsafe);
		for (unsigned w=1;w<120;w++) assert(f.io.core[w]->resets.size()==2 && f.io.core[w]->coreReset==0x46800);
	}
	for (unsigned fault=1;fault<=10;fault++) {
		RunFixture f(argv[1],7); f.io.rootFault=fault; assert(!f.Run());
		assert(f.lease->unsafe && f.banks.Quarantined() && f.job->result.recovery_required && f.job->result.state!=TT_RUNTIME_PASSED);
		if (fault<=7 || fault==10) assert(!f.io.rootStops && !f.io.rootHolds);
		if (fault==3 || fault==4 || fault==5 || fault==7 || fault==10) assert(!f.io.outputReads);
		for (const auto& rows:f.job->output) for (unsigned value:rows) assert(!value);
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true; assert(!f.Run());
		assert(f.io.Writes()==writes && f.io.selects==selects);
	}
	{
		RunFixture f(argv[1],7); f.initialized[0]=0; unsigned writes=f.io.Writes(),selects=f.io.selects;
		assert(!f.Run() && !f.lease->unsafe && f.io.Writes()==writes && f.io.selects==selects);
	}
	puts("PASS OFFLINE persistent RUN controller+actual dispatcher:1/7/119 peers xQ8/Q4/Q6 twice,host history ALL64 stats/perpeer mutations+duplicate replies refused,no peer resets,ALL authority before IO,ALL metadata before output reads,ALL proofs before root HOLD,10 late faults zero output/quarantine ALL/no retry/reset/restore/free;NO native card/inference qualification");
}
#endif
