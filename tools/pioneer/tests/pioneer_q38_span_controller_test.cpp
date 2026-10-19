/* SPDX-License-Identifier: MIT */
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#define PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
#include "pioneer_q38_panel_probe_test.cpp"
#define PIONEER_Q38_SPAN_MACHINE_ONLY
#include "pioneer_q38_span_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanController.h"
// Real controller AND compiled RV32 instructions. Only hardware/memory is
// simulated; this does not qualify a native driver or grant DMA authority.
struct SpanControlIO:PanelIO {
	const char* image;
	std::vector<uint32_t> wideWords=std::vector<uint32_t>(TT_WIDE_OUTPUT_WORDS,0xabcdef00);
	uint64_t widePhysical=UINT64_C(0x140000000),clock=0;
	unsigned touches=0,pciWrites=0,rootStarts=0,rootStops=0,rootHolds=0,pcSamples=0;
	unsigned wideLoads=0;
	std::vector<uint32_t> lastDeviceWords;
	unsigned packetFault=0,summaryFault=~0U,descriptorFault=~0U,commandFault=~0U;
	unsigned dropPci=0,dropReset=0,dropEntry=0,entryWrites=0;
	unsigned latePcFault=0,sourceFault=~0U,lateWideFault=~0U,initialWideFault=~0U;
	unsigned deviceDropPacket=~0U,deviceCorruptPacket=~0U;
	bool activeCohort=false,othersHeld=true,capacity=true,wideAllocated=false,wideCached=true;
	bool wideDirty=false,wideQuarantined=false,wideReleased=false,wideMapLost=false;
	bool noCompletion=false,corruptCode=false,corruptCounter=false,bulkMapLost=false;
	bool coreMapLost=false,authorityChanged=false,stageLost=false,lateMapLoss=false,lateCohort=false;
	std::vector<uint8_t>* initialized=nullptr;
	SpanControlIO(const char* path):image(path) {}
	void Touch() { assert(!HostReadMock::quarantined && !wideQuarantined); touches++; }
	bool SpanBeforeCohort() { return !activeCohort; } // metadata ONLY
	bool MapBulk() { Touch(); return !bulkMapLost; }
	bool MapQ38Panel() { Touch(); return !coreMapLost && mapping; }
	bool SpanViewCapacity(unsigned words) { return capacity && words<=saved.size(); }
	bool SpanOtherCoresHeld() { Touch(); return othersHeld; } // mock physical proof; NO reset
	bool SelectRuntimeCore(unsigned w) { Touch(); return w==0; }
	bool Q38PanelMappingStable() { Touch(); return !coreMapLost && mapping; }
	bool BulkMappingStable() { Touch(); return !bulkMapLost; }
	uint32_t Q38PanelRead(uint32_t a) { Touch(); return PanelIO::Q38PanelRead(a); }
	void Q38PanelWrite(uint32_t a,uint32_t v) { Touch(); PanelIO::Q38PanelWrite(a,v); }
	uint32_t RuntimeCodeRead(unsigned w) { Touch(); return PanelIO::RuntimeCodeRead(w); }
	void RuntimeCodeWrite(unsigned w,uint32_t v) { Touch(); if (++entryWrites!=dropEntry) PanelIO::RuntimeCodeWrite(w,v); }
	uint32_t RuntimeStatusRead(unsigned w) { Touch(); return PanelIO::RuntimeStatusRead(w); }
	void RuntimeStatusWrite(unsigned w,uint32_t v) { Touch(); PanelIO::RuntimeStatusWrite(w,v); }
	uint32_t ResetRead() { Touch(); return coreReset; }
	uint32_t RuntimePcRead() { Touch(); pcSamples++; return latePcFault && pcSamples==latePcFault?0x72004:
		(pcSamples&1?TT_Q38_SPAN_PARK:TT_Q38_SPAN_PARK_FETCH); }
	uint32_t ResidentNocRead(uint32_t a) { Touch(); return PanelIO::ResidentNocRead(a); }
	uint32_t BulkL1Read(unsigned w) { Touch(); return BulkMock::BulkL1Read(w); }
	void BulkL1Write(unsigned w,uint32_t v) { Touch(); BulkMock::BulkL1Write(w,v); }
	uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) { Touch(); return StoreMock::HostPciRead(p,o,s); }
	void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t v) { Touch(); if (++pciWrites!=dropPci) StoreMock::HostPciWrite(p,o,s,v); }
	int AllocateWideOutput() { assert(!wideAllocated); wideAllocated=true; return 0; }
	int WideOutputMap(uint64_t& at,uint64_t& bytes) { Touch(); assert(wideAllocated && !wideReleased);
		at=widePhysical+(wideMapLost?TT_WIDE_OUTPUT_BYTES:0); bytes=TT_WIDE_OUTPUT_BYTES; return 0; }
	int WideOutputPageMap(unsigned p,uint64_t& at,uint64_t& bytes) { Touch(); assert(p<256 && wideAllocated && !wideReleased);
		at=widePhysical+p*4096; bytes=4096; return 0; }
	int SetWideOutputNonCached() { assert(wideAllocated); wideCached=false; return 0; }
	void WideHostFence() { Touch(); wideDirty=false; }
	void WideHostStore(unsigned w,uint32_t v) { Touch(); assert(wideAllocated && !wideReleased && !wideCached && coreReset==0x47800);
		wideWords.at(w)=v; wideDirty=true; }
	uint32_t WideHostLoad(unsigned w) { Touch(); wideLoads++; assert(wideAllocated && !wideReleased && !wideCached && !wideDirty);
		return wideWords.at(w)^((!rootStarts && w==initialWideFault) || (rootStarts && w==lateWideFault)?1:0); }
	int ReleaseWideOutput() { Touch(); assert(!wideReleased); wideReleased=true; return 0; }
	void QuarantineWideOutput() { wideQuarantined=true; }
	void Pause() { Touch(); clock+=10000; }
	uint64_t Q38CohortNow() { return clock; }
	void Execute() {
		Q38SpanMachine m(image,l1[4],l1[10]);
		m.base=l1[5]; m.hostLow=l1[7]; m.hostHigh=l1[8];
		m.hostData.assign(wideWords.begin(),wideWords.begin()+m.bytes/4); m.golden=m.hostData;
		for (unsigned w=0;w<view.size();w++) {
			uint32_t at=TT_Q38_SPAN_VIEW_BASE+w*4; m.memory[at]=view[w];
			if (m.CodeAddress(at)) m.l1[m.Index(at)]=view[w];
		}
		m.l1[m.Index(0x6000)]=entry;
		for (unsigned w=0;w<8192;w++) m.memory[0x140000+w*4]=l1[w];
		for (unsigned w=0;w<64;w++) m.descriptor[w]=l1[w];
		for (unsigned w=0;w<16;w++) m.seals[w]=w?source[w]:config;
		for (unsigned w=0;w<8;w++) m.counts[w]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(w));
		for (unsigned w=0;w<10;w++) m.command[w]=command[w];
		m.failPacket=packetFault; m.dropPacket=int(deviceDropPacket); m.corruptPacket=int(deviceCorruptPacket);
		(void)m.Run();
		lastDeviceWords=m.gddr;
		std::copy(m.hostData.begin(),m.hostData.end(),wideWords.begin());
		for (unsigned w=0;w<view.size();w++) view[w]=m.memory.at(TT_Q38_SPAN_VIEW_BASE+w*4);
		for (unsigned w=0;w<8192;w++) l1[w]=m.memory.at(0x140000+w*4);
		for (unsigned w=0;w<3;w++) destination[2032+w]=m.memory.at(0x17ffc0+w*4);
		counters[0]=m.counts[0]; counters[1]=m.counts[1]; accepted=m.counts[2]; counters[3]=m.counts[3];
		writeCounters[0]=m.counts[4]; writeCounters[1]=m.counts[5]; writeCounters[3]=m.counts[7];
		for (unsigned w=0;w<10;w++) command[w]=m.command[w];
		// Hardware-fabric fault injection AFTER actual instruction execution.
		if (summaryFault<32) l1[3072+summaryFault]^=1;
		if (descriptorFault<64) l1[descriptorFault]^=1;
		if (commandFault<10) command[commandFault]^=1;
		if (sourceFault<16) { if (sourceFault) source[sourceFault]^=1; else config^=1; }
		if (corruptCode) view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		if (corruptCounter) counters[0]++;
		if (authorityChanged) { assert(initialized); (*initialized)[0]=1; }
		if (stageLost) destination[2034]=TT_Q38_SPAN_FAULT;
		if (lateMapLoss) wideMapLost=true;
		if (lateCohort) activeCohort=true;
	}
	void RuntimeResetWrite(uint32_t value) {
		Touch(); resets.push_back(value); if (resets.size()==dropReset) return;
		if (value==0x87ffff) { if (coreReset==0x46800) rootStops++; coreReset=value; return; }
		if (value==0x47800) { rootHolds++; coreReset=value; return; }
		assert(value==0x46800 && coreReset==0x87ffff && othersHeld && !activeCohort
			&& pciCommand[0]==0x406 && pciCommand[1]==0x406 && !wideDirty);
		coreReset=value; rootStarts++; if (!noCompletion) Execute();
	}
};
struct SpanControlFixture {
	SpanControlIO io; PanelOwners owner;
	PioneerOutputBufferLease transfer; PioneerBulkUploadLease bulk; PioneerWideOutputLease wide;
	PioneerQ38SpanLease lease; PioneerQ38SpanResult result;
	std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
	explicit SpanControlFixture(const char* image):io(image),owner(io) {
		assert(PioneerPrepareOutputBuffer(io,owner.host,transfer));
		assert(PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk));
		assert(PioneerPrepareWideOutput(io,owner.host,transfer,wide));
		io.initialized=&initialized;
	}
	bool PrepareSpan() { return PioneerQ38SpanPrepare(io,owner.dram,owner.noc,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease); }
	bool Upload(unsigned bytes=4096,unsigned bank=0,uint64_t offset=0,unsigned words=~0U) {
		std::vector<uint32_t> sourceWords(bytes/4,0);
		for (unsigned w=0;w<sourceWords.size();w++) sourceWords[w]=0x7fc00000U+w*0x9e3779b9U;
		return PioneerQ38SpanUpload(io,owner.dram,owner.noc,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,
			initialized.data(),unsigned(initialized.size()),bank,offset,bytes,sourceWords.data(),words==~0U?bytes/4:words,result);
	}
	bool CloseSpan(bool clean=true) { return PioneerQ38SpanClose(io,owner.dram,owner.noc,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,clean); }
	void Unsafe() {
		assert(lease.unsafe && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
			&& transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined
			&& !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
		unsigned count=io.touches;
		assert(!Upload() && !PrepareSpan() && !CloseSpan() && !CloseSpan(false));
		assert(!PioneerReleaseWideOutput(io,wide,false) && !PioneerReleaseOutputBuffer(io,transfer,false));
		assert(io.touches==count && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
	}
	void Finish() {
		assert(CloseSpan() && lease.closed && !lease.unsafe && io.coreReset==0x47800
			&& !wide.pending && !bulk.dirty && !io.HostReadMock::quarantined);
		for (unsigned w=0;w<io.view.size();w++) assert(io.view[w]==0xab860000+w);
		for (unsigned w=0;w<8192;w++) assert(io.l1[w]==0x91730000+w);
		assert(io.entry==0x1250006f);
		unsigned count=io.touches; assert(CloseSpan() && !PrepareSpan() && !Upload() && io.touches==count);
		assert(PioneerReleaseWideOutput(io,wide,true) && PioneerReleaseOutputBuffer(io,transfer,true));
		assert(PioneerResidentLeaseEnd(io,owner.noc,owner.store) && !io.ReleaseHostPage()); io.Safe();
	}
};
#ifndef PIONEER_Q38_SPAN_SERVICE_MOCK_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	{
		SpanControlFixture f(argv[1]);
		assert(PioneerQ38SpanSources(f.io,f.owner.dram,f.owner.noc,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,0x47800,0x402));
		assert(f.PrepareSpan());
		for (unsigned bank=0;bank<7;bank++) for (unsigned bytes:{4096U,20480U,1048576U}) {
			assert(f.Upload(bytes,bank,PioneerResidentBanks::Capacity-bytes));
			assert(f.result.state==TT_RUNTIME_PASSED && f.result.pages==bytes/4096 && f.result.host_exact
				&& f.result.matched_words==bytes/4 && !f.wide.pending && f.io.pciCommand[0]==0x402 && f.io.pciCommand[1]==0x402);
		}
		assert(f.lease.nonce==21 && f.io.rootStarts==21 && f.io.rootStops==21 && f.io.rootHolds==21);
		assert(std::count(f.initialized.begin(),f.initialized.end(),uint8_t(1))==0); // controller NEVER commits authority
		f.Finish();
	}
	puts("PASS actual span controller all7 banks/1MiB edges,21 jobs,physical echo/full restoration/private bitmap unchanged"); fflush(stdout);
	{
		SpanControlFixture f(argv[1]);
		// Metadata remains owner-bound:change the physical owner BEFORE its
		// real preparation,not by forging the controller's descriptor.
		assert(PioneerReleaseWideOutput(f.io,f.wide,true));
		f.io.wideAllocated=false; f.io.wideReleased=false; f.wide=PioneerWideOutputLease{};
		f.io.widePhysical=UINT64_C(0xfffffff00000);
		assert(PioneerPrepareWideOutput(f.io,f.owner.host,f.transfer,f.wide));
		assert(f.PrepareSpan());
		for (unsigned i=0;i<4;i++) f.io.counters[i]=f.io.writeCounters[i]=0xfffffff0;
		f.io.accepted=0xfffffff0;
		assert(f.Upload(1048576,6,PioneerResidentBanks::Capacity-1048576)); f.Finish();
	}
	for (unsigned word=0;word<32;word++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.summaryFault=word;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds && f.io.pciWrites==2); f.Unsafe();
	}
	for (unsigned word=0;word<64;word++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.descriptorFault=word;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned word=0;word<10;word++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.commandFault=word;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned packet=1;packet<=7;packet++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.packetFault=packet;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds && f.result.marker==TT_Q38_SPAN_FAULT); f.Unsafe();
	}
	for (unsigned packet=0;packet<7;packet++) for (bool corrupt:{false,true}) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan());
		if (corrupt) f.io.deviceCorruptPacket=packet; else f.io.deviceDropPacket=packet;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned fault=0;fault<16;fault++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.sourceFault=fault;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned fault=0;fault<15;fault++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan());
		if (!fault) f.io.noCompletion=true; if (fault==1) f.io.corruptCode=true;
		if (fault==2) f.io.corruptCounter=true; if (fault==3) f.io.lateWideFault=512;
		if (fault==4) f.io.initialWideFault=512; if (fault==5) f.io.authorityChanged=true;
		if (fault==6) f.io.stageLost=true; if (fault==7) f.io.wideMapLost=true;
		if (fault==8) f.io.bulkMapLost=true; if (fault==9) f.io.coreMapLost=true;
		if (fault==10) f.io.othersHeld=false; if (fault==11) f.io.latePcFault=3;
		if (fault==12) f.io.latePcFault=4;
		if (fault==13) f.io.lateMapLoss=true; if (fault==14) f.io.lateCohort=true;
		assert(!f.Upload() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned drop=1;drop<=4;drop++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.dropPci=drop;
		assert(!f.Upload() && f.io.pciWrites==drop && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
	}
	for (unsigned drop=1;drop<=4;drop++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.dropReset=drop;
		assert(!f.Upload() && f.io.resets.size()==drop); f.Unsafe();
	}
	puts("PASS span unknown packet/source/counter/descriptor/command/PC/echo/BME/reset faults;ALL owners retained,NO later IO"); fflush(stdout);
	for (unsigned fault=0;fault<9;fault++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan());
		unsigned bank=0,bytes=4096,words=1024; uint64_t offset=0;
		if (!fault) f.io.activeCohort=true; if (fault==1) f.initialized[0]=1;
		if (fault==2) bank=7; if (fault==3) offset=1;
		if (fault==4) offset=PioneerResidentBanks::Capacity; if (fault==5) words--;
		if (fault==6) bytes=4097; if (fault==7) f.lease.nonce=0x7ffffffe;
		if (fault==8) f.owner.dram.translated[6][1]^=1;
		unsigned count=f.io.touches; assert(!f.Upload(bytes,bank,offset,words) && !f.lease.unsafe && f.io.touches==count);
		// Refused caller authority does not damage the owned image.
	}
	for (unsigned fault=0;fault<7;fault++) {
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan() && f.Upload());
		if (!fault) f.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		if (fault==1) f.io.dropWrite=f.io.panelWrites+(0x72000-TT_Q38_SPAN_VIEW_BASE)/4+1;
		if (fault==2) { f.io.l1[0]^=1; f.io.dropBulk=f.io.bulkWrites+1; }
		if (fault==3) f.io.dropEntry=f.io.entryWrites+1;
		if (fault==4) f.io.othersHeld=false;
		if (fault==5) f.io.source[11]^=1;
		assert(!f.CloseSpan(fault!=6)); f.Unsafe();
	}
	{
		SpanControlFixture f(argv[1]); f.io.capacity=false; unsigned count=f.io.touches;
		assert(!f.PrepareSpan() && !f.lease.unsafe && count==f.io.touches);
		assert(f.CloseSpan() && f.lease.closed && !f.PrepareSpan() && f.io.touches==count);
	}
	for (unsigned fault=0;fault<3;fault++) {
		SpanControlFixture f(argv[1]);
		if (!fault) f.io.dropWrite=1; if (fault==1) f.io.dropEntry=1;
		if (fault==2) f.io.othersHeld=false;
		assert(!f.PrepareSpan() && !f.io.rootStarts); f.Unsafe();
	}
	{
		SpanControlFixture f(argv[1]); assert(f.PrepareSpan()); f.io.dropBulk=f.io.bulkWrites+1;
		assert(!f.Upload() && !f.io.rootStarts && !f.io.pciWrites); f.Unsafe();
	}
	puts("PASS OFFLINE actual span controller:metadata refusal before IO;single-attempt restore/close;UNKNOWN retains full context and all3 host owners;NO native driver/ABI/inference/speed qualification");
}
#endif
