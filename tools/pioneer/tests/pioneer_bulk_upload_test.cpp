/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#define PIONEER_OUTPUT_BUFFER_MOCK_ONLY
#include "pioneer_output_buffer_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerBulkUpload.h"
#include <vector>
struct BulkMock:StoreMock,OutputBufferMock {
	uint32_t l1[8192],saved[8192],gddr[8192];
	unsigned bulkWrites=0,dropBulk=0,fences=0,hostReads=0,hostStores=0;
	unsigned dropHost=0,corruptWord=~0U,lateWord=~0U;
	bool hostDirty=false,badBulkMap=false,dropPosted=false,partialPosted=false,latePosted=false;
	BulkMock() {
		for (unsigned w=0;w<8192;w++) { l1[w]=0x91730000+w; gddr[w]=0x24680000+w; }
	}
	uint32_t* BulkSavedWords() { return saved; }
	bool BulkMappingStable() { return !badBulkMap; }
	uint32_t BulkL1Read(unsigned w) { assert(w<8192); return l1[w]; }
	void BulkL1Write(unsigned w,uint32_t value) { assert(!uncertain && w<8192); if (++bulkWrites!=dropBulk) l1[w]=value; }
	void BulkHostFence() { fences++; hostDirty=false; }
	void BulkHostStore(unsigned w,uint32_t value) {
		assert(!uncertain && OutputBufferMock::allocated && !OutputBufferMock::cached && w<4096);
		if (++hostStores!=dropHost) OutputBufferMock::data[w]=value;
		hostDirty=true;
	}
	uint32_t BulkHostLoad(unsigned w) {
		assert(!hostDirty && OutputBufferMock::allocated && !OutputBufferMock::cached && w<4096); hostReads++;
		return OutputBufferMock::data[w]^((doorbells>=6 && w==lateWord)?1:0);
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset!=0x40) { StoreMock::ResidentNocWrite(offset,value); return; }
		assert(!uncertain && !pending && !hostDirty && value==1 && config==0x16005);
		doorbells++; doorbellRead=false;
		if (++Mock::writes==Mock::drop) { uncertain=true; return; }
		requests++; pending=true; done=false;
		if (failRequest==doorbells) { uncertain=true; return; }
		assert(command[6]==0 && command[9]==0 && command[8]>=4096 && command[8]<=16384 && !(command[8]&4095));
		unsigned words=command[8]/4,flits=command[8]/64;
		bool writing=command[7]==0x212;
		if (writing && command[5]==(19|(24<<6))) {
			assert(command[0]==TT_BULK_RETURN_L1 && command[1]==0 && command[2]==(1|(2<<6)));
			assert(command[3]==uint32_t(OutputBufferMock::physical) && command[4]==uint32_t(OutputBufferMock::physical>>32));
			assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
			if (!dropPosted) for (unsigned w=0;w<words-(partialPosted?1:0);w++) OutputBufferMock::data[w]=l1[4096+w]^(w==corruptWord?1:0);
		} else if (writing) {
			assert(command[0]==TT_BULK_UPLOAD_L1 && command[1]==0 && command[2]==(1|(2<<6)));
			assert(command[4]==0 && command[5]==(18|(12<<6)) && pciCommand[0]==0x402 && pciCommand[1]==0x402);
			unsigned at=(command[3]-0x04000000)/4; assert(at+words<=8192);
			for (unsigned w=0;w<words;w++) gddr[at+w]=l1[w];
		} else if (command[2]==(19|(24<<6))) {
			assert(command[0]==uint32_t(OutputBufferMock::physical) && command[1]==uint32_t(OutputBufferMock::physical>>32));
			assert(command[3]==TT_BULK_UPLOAD_L1 && command[4]==0 && command[5]==(1|(2<<6)) && command[7]==0);
			assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
			for (unsigned w=0;w<words;w++) l1[w]=OutputBufferMock::data[w];
		} else {
			assert(command[2]==(18|(12<<6)) && command[1]==0 && command[3]==TT_BULK_RETURN_L1
				&& command[4]==0 && command[5]==(1|(2<<6)) && command[7]==0);
			unsigned at=(command[0]-0x04000000)/4; assert(at+words<=8192);
			for (unsigned w=0;w<words;w++) l1[4096+w]=gddr[at+w];
		}
		uint32_t* count=writing?writeCounters:counters;
		count[0]++; count[1]+=flits+(badFlits?1:0); accepted++; count[3]++;
		pending=false; done=true;
	}
};
struct Owners {
	tt_host_page_result input={}; PioneerResidentLease store; PioneerOutputBufferLease host; PioneerBulkUploadLease bulk;
};
static bool Prepare(BulkMock& io,Owners& own)
{
	return Prepared(io,own.input,own.store) && PioneerPrepareOutputBuffer(io,own.input,own.host)
		&& PioneerBulkUploadPrepare(io,io.Baseline(),io.Compute(),own.input,own.store,own.host,own.bulk);
}
static bool Upload(BulkMock& io,Owners& own,const uint32_t* data,unsigned pages,tt_bulk_upload_result& r,uint64_t offset=0)
{
	return PioneerBulkUpload(io,io.Baseline(),io.Compute(),own.input,own.store,own.host,own.bulk,32768,offset,pages,data,r);
}
static void Close(BulkMock& io,Owners& own,bool clean)
{
	assert(PioneerBulkUploadClose(io,io.Baseline(),io.Compute(),own.input,own.store,own.host,own.bulk)==clean);
	if (clean) {
		for (unsigned w=0;w<8192;w++) assert(io.l1[w]==0x91730000+w);
		assert(PioneerReleaseOutputBuffer(io,own.host,true));
		assert(PioneerResidentLeaseEnd(io,io.Compute(),own.store) && io.ReleaseHostPage()==0); io.Safe();
	} else assert(own.bulk.unsafe && io.HostReadMock::quarantined && io.OutputBufferMock::quarantined
		&& !PioneerReleaseOutputBuffer(io,own.host,false) && !io.OutputBufferMock::releases && !io.PageMock::releases);
}
#ifndef PIONEER_BULK_UPLOAD_MOCK_ONLY
int main()
{
	(void)&Input;
	uint32_t data[4096]; for (unsigned w=0;w<4096;w++) data[w]=0x12345678^(w*0x3141);
	unsigned controls=0;
	for (unsigned pages:{1U,2U,3U,4U}) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		assert(Upload(io,own,data,pages,r,4096));
		assert(r.pages==pages && r.host_words==pages*1024 && r.transfer.completed==6
			&& r.transfer.host_visible && r.transfer.readback_complete && !r.transfer.mismatches && !r.transfer.recovery_required);
		for (unsigned w=0;w<8192;w++) assert(io.gddr[w]==(w>=1024 && w<1024+pages*1024?data[w-1024]:0x24680000+w));
		controls=io.Mock::writes; assert(io.fences>=12 && io.hostReads>=pages*1024*4 && io.hostStores==pages*2048);
		Close(io,own,true);
	}
	for (unsigned drop=1;drop<=controls;drop++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		if (drop<=io.Mock::writes) continue;
		io.Mock::drop=drop; bool passed=Upload(io,own,data,4,r);
		if (passed) assert(!r.transfer.recovery_required);
		Close(io,own,!r.transfer.recovery_required);
	}
	for (unsigned drop=1;drop<=8192;drop++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own)); io.dropHost=drop;
		assert(!Upload(io,own,data,4,r) && r.transfer.mismatches && !r.transfer.host_write_issued && !r.transfer.recovery_required);
		Close(io,own,true);
	}
	for (unsigned word=0;word<4096;word++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own)); io.lateWord=word;
		assert(!Upload(io,own,data,4,r) && r.transfer.recovery_required && r.transfer.readback_complete);
		Close(io,own,false);
	}
	for (unsigned step=1;step<=6;step++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own)); io.failRequest=step;
		assert(!Upload(io,own,data,4,r) && r.transfer.recovery_required && r.transfer.requests==step);
		Close(io,own,false);
	}
	for (unsigned word=0;word<8192;word++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		assert(Upload(io,own,data,4,r));
		// Make every restored word observably different, including the unused
		// tail of each fixed region. Any lost restoration retains both owners.
		for (unsigned w=0;w<8192;w++) io.l1[w]=~io.saved[w];
		io.dropBulk=word+1; Close(io,own,false);
	}
	{
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		for (unsigned w=0;w<4;w++) io.counters[w]=io.writeCounters[w]=UINT32_MAX-2;
		assert(Upload(io,own,data,4,r)); Close(io,own,true);
	}
	for (unsigned fault=0;fault<9;fault++) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		io.changed=fault==0; io.lateMapChange=fault==1; io.clearFails=fault==2; io.badFlits=fault==3;
		io.pciePending=fault==4; io.badBulkMap=fault==5; io.dropPosted=fault==6; io.partialPosted=fault==7;
		if (fault==8) io.corruptWord=1731;
		assert(!Upload(io,own,data,4,r) && r.transfer.recovery_required); Close(io,own,false);
	}
	for (unsigned pages:{0U,5U,~0U}) {
		BulkMock io; Owners own; tt_bulk_upload_result r; assert(Prepare(io,own));
		assert(!Upload(io,own,data,pages,r) && !io.requests); Close(io,own,true);
	}
	assert(!PioneerBulkUploadRange(32768,UINT64_MAX-4095,4) && !PioneerBulkUploadRange(32768,24576,3)
		&& !PioneerBulkUploadRange(32768,1,4) && PioneerBulkUploadRange(32768,16384,4));
	puts("PASS: OFFLINE 1..4-page owned bulk upload, fixed scratch restoration, six exact transfer proofs, all8192 dropped host stores/all4096 late word corruptions, control/completion/map/visibility/PCI failures, dual-owner quarantine; no hardware");
}
#endif
