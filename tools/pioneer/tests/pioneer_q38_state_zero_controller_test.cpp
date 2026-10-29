/* SPDX-License-Identifier: MIT */
#include <memory>
#include <set>
#define PIONEER_Q38_SPAN_SERVICE_MOCK_ONLY
#include "pioneer_q38_span_controller_test.cpp"
#define PIONEER_Q38_STATE_ZERO_HAS_BLOCK_MACHINE
#define PIONEER_Q38_STATE_ZERO_MACHINE_ONLY
#include "pioneer_q38_state_zero_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroController.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "../PioneerQ38TokenFifoQuiescence.h"
#endif
struct ZeroControlIO:SpanControlIO {
	using SpanControlIO::SpanControlIO;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
 // Legacy controller fixtures are also instantiated by the native TEST TU.
 // Observe the synchronous endpoint's CURRENT configuration,not source[0].
 // The actual native IO hides these methods with its FD/phase-scoped readers.
 uint32_t TokenFifoStatusWord(uint32_t address) {
  Touch();return NativeFifoStatusWord(address);
 }
 bool TokenFifoReadChecked(unsigned worker,bool overflow,uint32_t& value) {
  assert(!worker);value=TokenFifoStatusWord(overflow?0x68U:0x64U);return true;
 }
 bool TokenInitializationFifoEmpty(unsigned kind) {
  assert(kind<3);return PioneerQ38TokenFifoEmpty(*this,0,config);
 }
#endif
	bool competingSpan=false,lateCompetingSpan=false,lateOtherHeld=false,lateCoreMapLoss=false;
	bool backwardsClock=false,frozenClock=false;
	bool publicationCounterFabricOnly=false;
	unsigned publicationFailJob=0;
	uint64_t frozenTime=100000;
	unsigned badMarker=0,badPhase=0,badCount=0;
	PioneerQ38StateZeroLease* boundLease=nullptr;
	std::vector<unsigned> statusOrder;
	bool StateZeroBeforeCohort() { return !activeCohort && !competingSpan; }
	bool StateZeroOtherCoresHeld() { Touch(); return othersHeld; }
	uint64_t Q38CohortNow() { return rootStarts && backwardsClock?0:rootStarts && frozenClock?frozenTime:clock; }
	uint32_t RuntimeStatusRead(unsigned word) {
		if (rootStarts && coreReset==0x46800) statusOrder.push_back(word);
		return SpanControlIO::RuntimeStatusRead(word);
	}
	void ExecuteZero() {
		if (publicationCounterFabricOnly) {
			// Explicit lifecycle-only synthetic fabric. Does NOT execute RV32
			// or simulate zero/readback DATA. Used only to exercise all73728
			// private ledger pages/publication transitions without pretending
			// to repeat the separate actual-image arithmetic qualification.
			if (rootStarts==publicationFailJob) return; // lifecycle-only timeout injection,NO fabricated completion
			uint32_t before[8],after[8];
			for (unsigned word=0;word<8;word++) before[word]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(word));
			unsigned bytes=l1[4],chunks=(bytes+16383)/16384,flits=bytes/64;
			const unsigned delta[]={chunks,flits,chunks*2,chunks,chunks,flits,chunks*2,chunks};
			for (unsigned word=0;word<8;word++) after[word]=before[word]+delta[word];
			counters[0]=after[0]; counters[1]=after[1]; accepted=after[2]; counters[3]=after[3];
			writeCounters[0]=after[4]; writeCounters[1]=after[5]; writeCounters[3]=after[7];
			for (unsigned word=0;word<32;word++) l1[3072+word]=0;
			const uint32_t first[]={chunks,flits,chunks,flits,bytes/4096,bytes,l1[6],TT_Q38_ZERO_MARKER};
			for (unsigned word=0;word<8;word++) { l1[3072+word]=first[word]; l1[3080+word]=before[word]; l1[3088+word]=after[word]; }
			const uint32_t tail[]={l1[7],l1[8],l1[5],0x84000,0x144000,1};
			for (unsigned word=0;word<6;word++) l1[3096+word]=tail[word];
			unsigned last=(bytes-1)&~16383U;
			const uint32_t nextCommand[]={l1[5]+last,0,l1[8],0x144000,0,129,0,0,bytes-last,0};
			for (unsigned word=0;word<10;word++) command[word]=nextCommand[word];
			destination[2032]=0x7200c; destination[2033]=bytes/4096; destination[2034]=TT_Q38_ZERO_MARKER;
			return;
		}
		StateZeroMachine m(image,l1[4],l1[7]); m.base=l1[5];
		for (unsigned word=0;word<view.size();word++) {
			uint32_t at=TT_Q38_SPAN_VIEW_BASE+word*4; m.memory[at]=view[word];
			if (m.CodeAddress(at)) m.l1[m.Index(at)]=view[word];
		}
		m.l1[m.Index(0x6000)]=entry;
		for (unsigned word=0;word<8192;word++) m.memory[0x140000+word*4]=l1[word];
		for (unsigned word=0;word<64;word++) m.descriptor[word]=l1[word];
		for (unsigned word=0;word<16;word++) m.seals[word]=word?source[word]:config;
		for (unsigned word=0;word<8;word++) m.counts[word]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(word));
		for (unsigned word=0;word<10;word++) m.command[word]=command[word];
		m.failPacket=packetFault; m.dropData=int(deviceDropPacket); m.corruptData=int(deviceCorruptPacket);
		(void)m.Run(); lastDeviceWords=m.gddr;
		for (unsigned word=0;word<view.size();word++) view[word]=m.memory.at(TT_Q38_SPAN_VIEW_BASE+word*4);
		for (unsigned word=0;word<8192;word++) l1[word]=m.memory.at(0x140000+word*4);
		for (unsigned word=0;word<3;word++) destination[2032+word]=m.memory.at(0x17ffc0+word*4);
		counters[0]=m.counts[0]; counters[1]=m.counts[1]; accepted=m.counts[2]; counters[3]=m.counts[3];
		writeCounters[0]=m.counts[4]; writeCounters[1]=m.counts[5]; writeCounters[3]=m.counts[7];
		for (unsigned word=0;word<10;word++) command[word]=m.command[word];
		if (summaryFault<32) l1[3072+summaryFault]^=1;
		if (descriptorFault<64) l1[descriptorFault]^=1;
		if (commandFault<10) command[commandFault]^=1;
		if (sourceFault<16) { if (sourceFault) source[sourceFault]^=1; else config^=1; }
		if (corruptCode) view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		if (corruptCounter) counters[0]++;
		if (authorityChanged) { assert(boundLease); boundLease->identity.modelEpoch++; }
		if (stageLost) destination[2034]=TT_Q38_ZERO_FAULT;
		if (lateMapLoss) wideMapLost=true;
		if (lateCoreMapLoss) coreMapLost=true;
		if (lateCohort) activeCohort=true;
		if (lateCompetingSpan) competingSpan=true;
		if (lateOtherHeld) othersHeld=false;
		if (badMarker) destination[2034]=0xdeadbeef;
		if (badPhase) destination[2032]^=1;
		if (badCount) destination[2033]^=1;
	}
	void RuntimeResetWrite(uint32_t value) {
		Touch(); resets.push_back(value); if (resets.size()==dropReset) return;
		if (value==0x87ffff) { if (coreReset==0x46800) rootStops++; coreReset=value; return; }
		if (value==0x47800) { rootHolds++; coreReset=value; return; }
		assert(value==0x46800 && coreReset==0x87ffff && othersHeld && !activeCohort && !competingSpan
			&& pciCommand[0]==0x402 && pciCommand[1]==0x402 && !wideDirty);
		coreReset=value; rootStarts++; if (!noCompletion) ExecuteZero();
	}
};
struct ZeroControlFixture {
	ZeroControlIO io; PanelOwners owner;
	PioneerOutputBufferLease transfer; PioneerBulkUploadLease bulk; PioneerWideOutputLease wide;
	PioneerQ38StateZeroLease lease; PioneerQ38StateZeroResult result;
	PioneerQ38GdnIdentity identity={1,2,3,4};
	std::unique_ptr<PioneerQ38GdnBanks> state=std::make_unique<PioneerQ38GdnBanks>();
	std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
	std::vector<PioneerQ38GdnRegion> regions;
	unsigned preparedWideLoads=0;
	explicit ZeroControlFixture(const char* image,bool striped=false):io(image),owner(io) {
		assert(PioneerPrepareOutputBuffer(io,owner.host,transfer));
		assert(PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk));
		assert(PioneerPrepareWideOutput(io,owner.host,transfer,wide));
		preparedWideLoads=io.wideLoads;
		if (striped) {
			unsigned used[7]={};
			for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) {
				unsigned first=0;
				for (unsigned stripe=0;stripe<7;stripe++) {
					unsigned heads=6+unsigned(stripe<6),bank=(layer-layer/4+slot+stripe)%7;
					regions.push_back({layer,slot,first,heads,bank,0x04000000+used[bank]});
					used[bank]+=heads*65536; first+=heads;
				}
			}
		} else for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) {
			unsigned index=unsigned(regions.size()),bank=index%7;
			unsigned offset=index>=89?unsigned(PioneerResidentBanks::Capacity-48*65536):0x04000000+(index/7)*48*65536;
			regions.push_back({layer,slot,0,48,bank,offset});
		}
		assert(state->Bind(owner.banks,owner.dram,identity,initialized.data(),unsigned(initialized.size()),regions.data(),unsigned(regions.size())));
		io.boundLease=&lease;
	}
	bool PrepareZero() { return PioneerQ38StateZeroPrepare(io,owner.dram,owner.noc,*state,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,lease); }
	bool Zero(unsigned region=0,unsigned first=0,unsigned pages=1) {
		return PioneerQ38StateZeroRun(io,owner.dram,owner.noc,*state,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,identity,region,first,pages,result);
	}
	bool CloseZero(bool clean=true) { return PioneerQ38StateZeroClose(io,owner.dram,owner.noc,*state,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,clean); }
	void Unsafe() {
		assert(lease.unsafe && state->Unsafe() && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
			&& transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined
			&& !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases && !io.pciWrites);
		unsigned progress=UINT32_MAX;
		assert(!state->InitializationProgress(identity,progress) && progress==UINT32_MAX);
		unsigned count=io.touches;
		assert(!Zero() && !PrepareZero() && !CloseZero() && !CloseZero(false));
		assert(!PioneerReleaseWideOutput(io,wide,false) && !PioneerReleaseOutputBuffer(io,transfer,false));
		assert(io.touches==count && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
	}
	void Finish() {
		assert(CloseZero() && lease.closed && !lease.unsafe && io.coreReset==0x47800 && !wide.pending
			&& !bulk.dirty && !io.HostReadMock::quarantined && !io.pciWrites && io.wideLoads==preparedWideLoads);
		for (unsigned word=0;word<io.view.size();word++) assert(io.view[word]==0xab860000+word);
		for (unsigned word=0;word<8192;word++) assert(io.l1[word]==0x91730000+word);
		assert(io.entry==0x1250006f);
		unsigned count=io.touches; assert(CloseZero() && !PrepareZero() && !Zero() && io.touches==count);
		assert(PioneerReleaseWideOutput(io,wide,true) && PioneerReleaseOutputBuffer(io,transfer,true));
		assert(PioneerResidentLeaseEnd(io,owner.noc,owner.store) && !io.ReleaseHostPage()); io.Safe();
	}
};
#ifndef PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,nullptr,_IOLBF,0);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	const auto code=Read((std::string(argv[1])+"/text.bin").c_str());
	for (unsigned word=0;word<2048;word++) assert(PioneerQ38StateZeroCodeWord(word)==(word*4<code.size()?Word(code,word*4):0));
	assert(PioneerQ38StateZeroCodeWord(2048)==UINT32_MAX);
	{
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); unsigned loads=f.io.wideLoads;
		std::set<std::pair<unsigned,unsigned>> provedPages;
		unsigned progress=UINT32_MAX; assert(f.state->InitializationProgress(f.identity,progress) && !progress);
		for (unsigned region:{0U,1U,2U,3U,4U,5U,6U,89U,90U,91U,92U,93U,94U,95U})
		for (unsigned pages:{1U,5U,256U}) {
			assert(f.Zero(region,768-pages,pages));
			assert(f.result.state==TT_RUNTIME_PASSED && f.result.pages==pages && f.result.bytes==pages*4096
				&& f.result.started && f.result.completed && f.result.held && !f.result.recovery_required && !f.wide.pending);
			for (auto word:f.io.lastDeviceWords) assert(!word);
			assert(f.io.wideLoads==loads && !f.io.pciWrites);
			for (unsigned page=768-pages;page<768;page++) provedPages.insert({region,page});
			assert(f.state->InitializationProgress(f.identity,progress) && progress==provedPages.size());
		}
		for (auto byte:f.initialized) assert(!byte);
		f.Finish();
	}
	for (unsigned word=0;word<32;word++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.summaryFault=word; assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe(); }
	for (unsigned word=0;word<64;word++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.descriptorFault=word; assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe(); }
	for (unsigned word=0;word<10;word++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.commandFault=word; assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe(); }
	for (unsigned word=0;word<16;word++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.sourceFault=word; assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe(); }
	for (unsigned packet=1;packet<=4;packet++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.packetFault=packet; assert(!f.Zero(0,0,8) && !f.io.rootStops && !f.io.rootHolds && f.io.statusOrder==std::vector<unsigned>{2}); f.Unsafe(); }
	for (unsigned fault=0;fault<3;fault++) { ZeroControlFixture f(argv[1]); assert(f.PrepareZero());
		if (!fault) f.io.badMarker=1; else if (fault==1) f.io.badPhase=1; else f.io.badCount=1;
		std::vector<unsigned> expected={2,0,1}; expected.resize(fault+1);
		assert(!f.Zero() && f.io.statusOrder==expected); f.Unsafe();
	}
	for (unsigned packet=0;packet<2;packet++) for (bool corrupt:{false,true}) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero());
		if (corrupt) f.io.deviceCorruptPacket=packet; else f.io.deviceDropPacket=packet;
		assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned fault=0;fault<20;fault++) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero());
		if (!fault) f.io.noCompletion=true; if (fault==1) f.io.corruptCode=true;
		if (fault==2) f.io.corruptCounter=true; if (fault==3) f.io.authorityChanged=true;
		if (fault==4) f.io.stageLost=true; if (fault==5) f.io.wideMapLost=true;
		if (fault==6) f.io.bulkMapLost=true; if (fault==7) f.io.coreMapLost=true;
		if (fault==8) f.io.othersHeld=false; if (fault>=9 && fault<=12) f.io.latePcFault=fault-8;
		if (fault==13) f.io.lateMapLoss=true; if (fault==14) f.io.lateCohort=true;
		if (fault==15) f.io.lateCoreMapLoss=true; if (fault==16) f.io.lateCompetingSpan=true;
		if (fault==17) f.io.lateOtherHeld=true; if (fault==18) f.io.activeCohort=true;
		if (fault==19) f.io.competingSpan=true;
		assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned drop=1;drop<=4;drop++) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.dropReset=drop;
		assert(!f.Zero() && f.io.resets.size()==drop); f.Unsafe();
	}
	for (unsigned fault=0;fault<3;fault++) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.clock=100000;
		if (!fault) f.io.clock=UINT64_MAX-10;
		if (fault==1) f.io.backwardsClock=true;
		if (fault==2) { f.io.noCompletion=true; f.io.frozenClock=true; }
		assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned fault=0;fault<7;fault++) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); unsigned region=0,first=0,pages=1;
		if (!fault) region=96; if (fault==1) first=768;
		if (fault==2) pages=0; if (fault==3) pages=257;
		if (fault==4) { first=767; pages=2; }
		if (fault==5) f.identity.modelEpoch++;
		if (fault==6) f.lease.nonce=0x7ffffffe;
		unsigned before=f.io.touches; assert(!f.Zero(region,first,pages) && !f.lease.unsafe && f.io.touches==before);
	}
	for (unsigned fault=0;fault<7;fault++) {
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero() && f.Zero());
		if (!fault) f.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		if (fault==1) f.io.dropWrite=f.io.panelWrites+(0x72000-TT_Q38_SPAN_VIEW_BASE)/4+1;
		if (fault==2) { f.io.l1[0]^=1; f.io.dropBulk=f.io.bulkWrites+1; }
		if (fault==3) f.io.dropEntry=f.io.entryWrites+1;
		if (fault==4) f.io.othersHeld=false;
		if (fault==5) f.io.source[11]^=1;
		assert(!f.CloseZero(fault!=6)); f.Unsafe();
	}
	for (unsigned fault=0;fault<3;fault++) {
		ZeroControlFixture f(argv[1]); if (!fault) f.io.dropWrite=1;
		if (fault==1) f.io.dropEntry=1; if (fault==2) f.io.othersHeld=false;
		assert(!f.PrepareZero() && !f.io.rootStarts); f.Unsafe();
	}
	{
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.io.dropBulk=f.io.bulkWrites+1;
		assert(!f.Zero() && !f.io.rootStarts); f.Unsafe();
	}
	{
		ZeroControlFixture f(argv[1]); f.io.capacity=false; unsigned before=f.io.touches;
		assert(!f.PrepareZero() && !f.lease.unsafe && f.io.touches==before);
		assert(f.CloseZero() && f.lease.closed && !f.PrepareZero() && f.io.touches==before);
	}
	{
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); f.lease.running=true;
		unsigned before=f.io.touches; assert(!f.CloseZero() && before==f.io.touches); f.Unsafe();
	}
	{
		ZeroControlFixture f(argv[1]); assert(f.PrepareZero()); unsigned loads=f.io.wideLoads;
		f.io.initialWideFault=f.io.lateWideFault=512;
		assert(f.Zero() && f.io.wideLoads==loads && !f.io.pciWrites); f.Finish();
	}
	puts("PASS OFFLINE state-zero controller+actual image:reserved owner/epoch only,ALL zero/readback/counter/source/descriptor/command/terminal proofs BEFORE hold,NO BME/host traffic/learned bitmap commit,UNKNOWN retains all leases and permits ZERO later IO;NO native/context-lifetime integration");
}
#endif
