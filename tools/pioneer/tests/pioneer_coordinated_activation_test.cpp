/* SPDX-License-Identifier: MIT */
#define PIONEER_BULK_UPLOAD_MOCK_ONLY
#include "pioneer_bulk_upload_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatedActivation.h"
struct ActivationMock:BulkMock {
	uint32_t activation[4096]={};
	unsigned lateHost=~0U;
	unsigned RuntimeBufferWords() const { return TT_Q8_BULK_SAVED_BUFFER_WORDS; }
	uint32_t BulkHostLoad(unsigned w) { return BulkMock::BulkHostLoad(w)^((doorbells && w==lateHost)?1:0); }
	void ResidentNocWrite(unsigned offset,uint32_t value) {
		if (offset!=0x40) { StoreMock::ResidentNocWrite(offset,value); return; }
		assert(!uncertain && !pending && !hostDirty && value==1 && config==0x16005);
		doorbells++; requests++; doorbellRead=false; pending=true; done=false;
		if (++Mock::writes==Mock::drop || failRequest==doorbells) { uncertain=true; return; }
		const uint32_t expected[]={uint32_t(OutputBufferMock::physical),uint32_t(OutputBufferMock::physical>>32),
			19|(24<<6),0x80000,0,129,0,0,16384,0};
		assert(!memcmp(command,expected,sizeof(expected)) && pciCommand[0]==0x406 && pciCommand[1]==0x406);
		for (unsigned i=0;i<4096;i++) activation[i]=OutputBufferMock::data[i];
		counters[0]++; counters[1]+=256+(badFlits?1:0); accepted++; counters[3]++;
		pending=false; done=true;
	}
};
static bool RunActivation(ActivationMock& io,Owners& owners,unsigned k,tt_bulk_upload_result& result)
{
	std::vector<uint32_t> vector(k/2),scales(k/32);
	for (unsigned w=0;w<vector.size();w++) vector[w]=0x40014001U^(w&31);
	for (unsigned w=0;w<scales.size();w++) scales[w]=0x3f800000U+w;
	PioneerRuntimeMatvecLease compute; compute.ready=compute.saved=compute.bulk=true; compute.buffer_words=TT_Q8_BULK_SAVED_BUFFER_WORDS;
	bool passed=PioneerCoordinatedActivation(io,io.Baseline(),io.Compute(),owners.input,owners.store,owners.host,owners.bulk,compute,k,vector.data(),scales.data(),result);
	if (result.transfer.recovery_required) assert(compute.unsafe);
	if (passed) {
		assert(result.pages==4 && result.host_words==4096 && result.transfer.requests==1 && result.transfer.completed==1);
		for (unsigned w=0;w<4096;w++) assert(io.activation[w]==PioneerCoordinatedActivationWord(w,k,vector.data(),scales.data()));
		for (unsigned w=0;w<4;w++) assert(uint32_t(result.transfer.after[0][w]-result.transfer.before[0][w])==(w==1?256:1));
		assert(io.pciCommand[0]==0x402 && io.pciCommand[1]==0x402 && io.doorbells==1);
	}
	return passed;
}
int main()
{
	unsigned controls=0;
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners));
		unsigned before=io.Mock::writes;
		assert(RunActivation(io,owners,k,result)); controls=io.Mock::writes-before;
		Close(io,owners,true);
	}
	for (unsigned dropped=1;dropped<=4096;dropped++) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners));
		for (auto& value:io.OutputBufferMock::data) value=0xdeadbeef;
		io.dropHost=dropped;
		assert(!RunActivation(io,owners,1024,result) && result.transfer.mismatches && !io.doorbells && !result.transfer.recovery_required);
		Close(io,owners,true);
	}
	for (unsigned word:{0U,511U,3071U,3072U,3263U,4095U}) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners)); io.lateHost=word;
		assert(!RunActivation(io,owners,6144,result) && result.transfer.recovery_required); Close(io,owners,false);
	}
	for (unsigned drop=1;drop<=controls;drop++) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners)); io.Mock::drop=io.Mock::writes+drop;
		(void)RunActivation(io,owners,1024,result); Close(io,owners,!result.transfer.recovery_required);
	}
	for (unsigned fault=0;fault<8;fault++) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners));
		io.changed=fault==0; io.lateMapChange=fault==1; io.clearFails=fault==2; io.badFlits=fault==3;
		io.pciePending=fault==4; io.badBulkMap=fault==5; io.failRequest=fault==6?1:0;
		if (fault==7) for (auto& value:io.counters) value=UINT32_MAX-2;
		bool passed=RunActivation(io,owners,1024,result);
		if (fault==7) assert(passed); else assert(!passed && result.transfer.recovery_required);
		Close(io,owners,!result.transfer.recovery_required);
	}
	for (unsigned field=0;field<6;field++) {
		ActivationMock io; Owners owners; tt_bulk_upload_result result; assert(Prepare(io,owners));
		uint32_t vector[512],scales[32]; for (auto& word:vector) word=0x40014001; for (auto& word:scales) word=0x3f800000;
		PioneerRuntimeMatvecLease compute; compute.ready=compute.saved=compute.bulk=true; compute.buffer_words=TT_Q8_BULK_SAVED_BUFFER_WORDS;
		if (field==0) compute.ready=false; if (field==1) compute.saved=false; if (field==2) compute.bulk=false;
		if (field==3) compute.buffer_words=8192; if (field==4) compute.running=true; if (field==5) compute.unsafe=true;
		assert(!PioneerCoordinatedActivation(io,io.Baseline(),io.Compute(),owners.input,owners.store,owners.host,owners.bulk,compute,1024,vector,scales,result)
			&& !io.hostStores && !io.doorbells);
		Close(io,owners,true);
	}
	puts("PASS OFFLINE one16KiB owned activation READ,all K/exact layout,all4096 dropped host stores,counter wrap,control/map/source/late-data/completion faults,dual-owner quarantine; no RISC start/reset/device access");
}
