/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
#include "pioneer_q38_dispatch_bootstrap_test.cpp"
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchRun.h"
#include "../PioneerQ38DispatchHost.h"
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
#include "pioneer_q38_compact_output_fabric.h"
#else
#include "pioneer_q38_output_fabric.h"
#endif
#endif
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
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	tt_host_page_result dmaInput={}; PioneerOutputBufferLease dmaBulk; PioneerWideOutputLease dmaWide;
	PioneerWideOutputLease* dmaPendingOwner=&dmaWide;
	std::vector<uint32_t> dmaData=std::vector<uint32_t>(TT_WIDE_OUTPUT_WORDS,0xdeadbeef);
	unsigned pci[2]={0x402,0x402},pciWrites=0,pciReads=0,failPciWrite=0,failPciRead=0;
	unsigned pciWriteReads=0,failPciWriteRead=0; bool pciWritePending=false;
	unsigned dmaPackets=0,failDma=0,dropDma=0,corruptDma=0,changeSourceDma=0,wideLoads=0,wideMaps=0,lateMap=0;
	int badDmaPage=-1;
	bool wideQuarantined=false;
	int HostPageMap(uint64_t& at,uint64_t& bytes) { at=UINT64_C(0x120000000); bytes=4096; return 0; }
	int OutputBufferMap(uint64_t& at,uint64_t& bytes) { at=UINT64_C(0x100100000); bytes=16384; return 0; }
	int OutputBufferPageMap(unsigned page,uint64_t& at,uint64_t& bytes) { at=UINT64_C(0x100100000)+page*4096; bytes=4096; return 0; }
	int AllocateWideOutput() { return 0; }
	int WideOutputMap(uint64_t& at,uint64_t& bytes) { at=UINT64_C(0x180000000)+(++wideMaps==lateMap?0x100000:0); bytes=TT_WIDE_OUTPUT_BYTES; return 0; }
	int WideOutputPageMap(unsigned page,uint64_t& at,uint64_t& bytes) { at=UINT64_C(0x180000000)+page*4096+(int(page)==badDmaPage?4096:0); bytes=4096; return 0; }
	int SetWideOutputNonCached() { return 0; }
	void WideHostFence() {}
	void WideHostStore(unsigned i,uint32_t value) { assert(!wideQuarantined && i<dmaData.size()); dmaData[i]=value; }
	uint32_t WideHostLoad(unsigned i) { assert(!wideQuarantined && i<dmaData.size()); wideLoads++; return dmaData[i]; }
	void QuarantineWideOutput() { wideQuarantined=true; }
	uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) {
		if (o==4 && s==2) {
			pciReads++; bool fault=pciReads==failPciRead;
			if (pciWritePending) { pciWritePending=false; fault |= ++pciWriteReads==failPciWriteRead; }
			return pci[p]^(fault?4:0);
		}
		return BootstrapIO::HostPciRead(p,o,s);
	}
	void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t value) {
		assert(p<2 && o==4 && s==2 && (value==0x402 || value==0x406));
		assert(dmaPendingOwner && dmaPendingOwner->pending); pciWritePending=true;
		if (++pciWrites!=failPciWrite) pci[p]=value;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
	struct DmaPayloadView {
		RunIO& io; uint32_t base;
		void PayloadFence() { assert(!io.quarantined); }
		uint32_t PayloadRead(unsigned w) { return io.Q38PanelRead(base+w*4); }
		void PayloadWrite(unsigned w,uint32_t value) { io.Q38PanelWrite(base+w*4,value); }
		uint64_t PayloadReadPair(unsigned w) { assert(!(w&1)); uint64_t low=PayloadRead(w); return low|(uint64_t(PayloadRead(w+1))<<32); }
		void PayloadWritePair(unsigned w,uint32_t low,uint32_t high) { assert(!(w&1)); PayloadWrite(w,low); PayloadWrite(w+1,high); }
	};
	DmaPayloadView Q38PayloadData(uint32_t a) { return DmaPayloadView{*this,a}; }
#endif
#endif
	unsigned profiles=0;
	void Q38DispatchProfile(unsigned type,unsigned inputs,unsigned peers,bool valid,const uint64_t*) {
		assert(type && inputs && peers && valid); profiles++;
	}
	const char* image; unsigned rootFault=0,rootArms=0,rootStops=0,rootHolds=0,outputReads=0,inputMode=0;
	uint32_t peerDone[119]={}; bool rootExecuted=false;
	bool peerStopped[119]={},stopExecuted=false;
	unsigned stopSlowWorker=0;
	unsigned stopSnapshotMode=0,pcSamples[120]={};
	bool asynchronousFault=false,workOptionalChecks=false; std::vector<unsigned> statusOrder;
	explicit RunIO(const char* path):image(path) {
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		dmaInput.version=1; dmaInput.state=TT_HOST_PAGE_READY; dmaInput.allocated=dmaInput.memory_type_set=dmaInput.map_stable=1;
		dmaInput.physical=UINT64_C(0x120000000); dmaInput.bytes=4096;
		dmaBulk.allocated=dmaBulk.ready=dmaBulk.memory_type_set=true; dmaBulk.physical=UINT64_C(0x100100000);
		dmaWide.allocated=dmaWide.ready=dmaWide.memory_type_set=true; dmaWide.physical=UINT64_C(0x180000000);
#endif
	}
	uint32_t RuntimeStatusRead(unsigned index) {
		uint32_t value=BootstrapIO::RuntimeStatusRead(index);
		if (!current && rootExecuted && asynchronousFault) {
			statusOrder.push_back(index);
			if (statusOrder.size()==1) {
				C().destination[2032]=0x59020205; C().destination[2033]=5; C().destination[2034]=TT_Q38_DISPATCH_FAULT;
			}
		}
		return value;
	}
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
		inputMode=c.view[base+7];
		uint32_t source[16]; for (unsigned i=0;i<16;i++) source[i]=i?c.source[i]:c.config;
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		CompactReturnMachine<ControllerMachine> m(image,n,type,k,source);
#else
		OutputReturnMachine<ControllerMachine> m(image,n,type,k,source);
#endif
		m.failDma=failDma; m.dropDma=dropDma; m.corruptDma=corruptDma; m.changeSourceDma=changeSourceDma;
#else
		ControllerMachine m(image,n,type,k,source);
#endif
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		// Controller fixtures intentionally use this original constant input;
		// the direct/cooperative image tests separately vary all257 states.
		for (auto& value:m.originalInputs) value=0x40014001;
#endif
		bool stop=c.view[base+2]==2; if (stop) m.Configure(1,true);
		m.stopSlowWorker=stopSlowWorker;
		m.stopSnapshotMode=stopSnapshotMode;
		for (unsigned word=0;word<c.view.size();word++) m.memory[TT_Q38_PANEL_VIEW_BASE+word*4]=c.view[word];
		for (unsigned i=0;i<8;i++) m.counts[i]=c.ResidentNocRead(PioneerQ38PanelCounter(i));
		for (unsigned w=0;w<119;w++) m.peers[w].requested=m.peers[w].finished=peerDone[w];
		bool success=m.RunRoot();
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		dmaPackets+=m.dmaPackets;
		if (m.dmaPackets) { assert(pci[0]==0x406 && pci[1]==0x406); std::copy(m.host.begin(),m.host.end(),dmaData.begin()); }
		if (!success) { c.destination[2034]=TT_Q38_DISPATCH_FAULT; rootExecuted=true; return; }
#else
		assert(success);
#endif
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
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
			|| (value.first>=0xa0000 && value.first<0xa0000+k*2)
#endif
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
		if (rootFault==11) {
			c.destination[2032]=0x54000102; c.destination[2033]=1; c.destination[2034]=0;
			asynchronousFault=true;
		}
	}
	void RuntimeResetWrite(uint32_t value) {
		if (current) { BootstrapIO::RuntimeResetWrite(value); return; }
		assert(!quarantined); auto& c=C(); c.resets.push_back(value);
		if (value==0x87ffff) {
			if (c.coreReset==0x46800) { rootStops++; assert(rootFault==0 || rootFault==8 || rootFault==9 || (workOptionalChecks && (rootFault==3 || rootFault==5 || rootFault==6))); if (rootFault==8) return; }
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
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
	// Odd compressed stage sizes MUST NOT widen the original even-word
	// post-execution input comparator or its 64-bit pair-read contract.
	for (unsigned words:{1707U,2048U,5803U}) {
		assert(PioneerQ38DispatchCompactStageRange(0xa0000,words));
		assert(!PioneerQ38DispatchStageRange(0xa0000,words));
		assert(!PioneerQ38DispatchReadRange(0xa0000,words));
		RunFixture f(argv[1],1);
		assert(!PioneerQ38DispatchInputCompare(f.io,0xa0000,f.job->activation,words,0));
	}
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	for (unsigned k:{5120U,6144U,17408U}) for (unsigned type:{8U,12U,14U}) {
		RunFixture f(argv[1],1,type,k); auto& cache=f.lease->inputReuse;
		assert(!cache.Matches(*f.job,0,1)); cache.Commit(*f.job,1,1);
		assert(cache.Matches(*f.job,1,1));
		for (unsigned other:{8U,12U,14U}) {
			f.job->type=other;
#ifdef PIONEER_Q38_REUSE_CROSS_QUANT
			assert(cache.Matches(*f.job,1,1)==(PioneerQ38CohortBlockInputs(type)==PioneerQ38CohortBlockInputs(other)));
#else
			assert(cache.Matches(*f.job,1,1)==(type==other));
#endif
		}
		f.job->type=type;
		for (unsigned w=0;w<k/2;w++) for (unsigned bit=0;bit<32;bit++) {
			f.job->activation[w]^=1U<<bit; assert(!cache.Matches(*f.job,1,1)); f.job->activation[w]^=1U<<bit;
		}
		unsigned padded=(k/PioneerQ38CohortBlockInputs(type)+15)&~15U;
		for (unsigned w=0;w<padded;w++) for (unsigned bit=0;bit<32;bit++) {
			f.job->scales[w]^=1U<<bit; assert(!cache.Matches(*f.job,1,1)); f.job->scales[w]^=1U<<bit;
		}
		assert(!cache.Matches(*f.job,2,1) && !cache.Matches(*f.job,1,2)
			&& !cache.Matches(*f.job,1,0) && !cache.Matches(*f.job,1,UINT64_MAX));
		cache.Commit(*f.job,1,UINT64_MAX); assert(!cache.valid);
		assert(f.Run() && f.io.inputMode==1 && f.Run() && f.io.inputMode==2);
		f.lease->inputEpoch++;
		assert(f.Run() && f.io.inputMode==1 && f.Run() && f.io.inputMode==2);
#ifdef PIONEER_Q38_REUSE_CROSS_QUANT
		if (type!=8) {
			f.job->type=type==12?14:12;
			assert(f.Run() && f.io.inputMode==2);
		}
#endif
	}
	for (unsigned fault:{2U,3U,4U,5U,6U,7U,10U}) {
		RunFixture f(argv[1],7); assert(f.Run()); f.io.rootFault=fault;
		unsigned outputs=f.io.outputReads,stops=f.io.rootStops;
		assert(!f.Run() && f.io.inputMode==2 && !f.lease->inputReuse.valid && f.lease->unsafe && f.banks.Quarantined());
		if (fault!=6) assert(f.io.outputReads==outputs);
		assert(f.io.rootStops==stops);
		for (const auto& rows:f.job->output) for (unsigned value:rows) assert(!value);
	}
	puts("PASS reuse controller:ALL active input/scale bits miss on mutation,all3 widths xQ8/Q4/Q6,nonce/epoch/saturation,actual root hits/misses,7 hit-path late faults preserve full proofs and quarantine without output/reset");
#endif
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
#ifdef TT_PIONEER_Q38_DISPATCH_PROFILE
		assert(f.io.profiles==2);
#else
		assert(!f.io.profiles);
#endif
		for (unsigned w=1;w<120;w++) assert(f.io.core[w]->resets.size()==2 && f.io.core[w]->coreReset==0x46800);
	}
	for (unsigned fault=1;fault<=10;fault++) {
		RunFixture f(argv[1],7); f.io.rootFault=fault; assert(!f.Run());
		assert(f.lease->unsafe && f.banks.Quarantined() && f.job->result.recovery_required && f.job->result.state!=TT_RUNTIME_PASSED);
		assert(!f.io.profiles); // no report after ANY uncertain operation
		if (fault<=7 || fault==10) assert(!f.io.rootStops && !f.io.rootHolds);
		if (fault==3 || fault==4 || fault==5 || fault==7 || fault==10) assert(!f.io.outputReads);
		for (const auto& rows:f.job->output) for (unsigned value:rows) assert(!value);
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true; assert(!f.Run());
		assert(f.io.Writes()==writes && f.io.selects==selects);
	}
	{
		RunFixture f(argv[1],2); f.io.rootFault=11; assert(!f.Run());
		assert(f.lease->unsafe && f.banks.Quarantined() && !f.io.outputReads && !f.io.rootStops && !f.io.rootHolds);
		assert(f.job->result.root_marker==TT_Q38_DISPATCH_FAULT);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
		assert((f.io.statusOrder==std::vector<unsigned>{2,0,1,2,0,1}));
		assert(f.job->result.root_phase==0x59020205 && f.job->result.root_count==5);
		puts("PASS59 marker-first ORIGINAL three-read sample captures stable fault detail;publication during first sample waits for next normal monitor sample;no output/reset/hold/extra diagnostic read");
#else
		assert((f.io.statusOrder==std::vector<unsigned>{0,1,2}));
		assert(f.job->result.root_phase==0x54000102 && f.job->result.root_count==5);
		puts("EXPECTED legacy diagnostic torn tuple reproduced:old checkpoint paired with new fault marker;no native claim");
#endif
	}
	{
		RunFixture f(argv[1],7); f.initialized[0]=0; unsigned writes=f.io.Writes(),selects=f.io.selects;
		assert(!f.Run() && !f.lease->unsafe && f.io.Writes()==writes && f.io.selects==selects);
	}
	puts("PASS OFFLINE persistent RUN controller+actual dispatcher:1/7/119 peers xQ8/Q4/Q6 twice,host history ALL64 stats/perpeer mutations+duplicate replies refused,no peer resets,ALL authority before IO,ALL metadata before output reads,ALL proofs before root HOLD,10 late faults zero output/quarantine ALL/no retry/reset/restore/free;NO native card/inference qualification");
}
#endif
