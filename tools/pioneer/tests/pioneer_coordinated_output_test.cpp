/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATED_FLEET_MOCK_ONLY
#include "pioneer_coordinated_fleet_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatedOutput.h"
struct OutputIO:CoordinatedIO {
	unsigned transfers=0,compareStarts=0,failTransfer=0,corruptPosted=~0U,corruptReadback=~0U,lateHost=~0U;
	unsigned hostStores=0,dropHost=0;
	unsigned controlStores=0,dropControlStore=0;
	bool dropPosted=false,partialPosted=false,badTransferCounter=false,badCompareCounter=false,compareMismatch=false;
	uint32_t BulkHostLoad(unsigned w) { return CoordinatedIO::BulkHostLoad(w)^((compareStarts && w==lateHost)?1:0); }
	void BulkHostStore(unsigned w,uint32_t value) { if (++hostStores!=dropHost) CoordinatedIO::BulkHostStore(w,value); }
	void QuarantineHostPage() { quarantined=true; core[0]->QuarantineHostPage(); }
	void ResidentNocWrite(unsigned offset,uint32_t value) {
		if (offset!=0x40) { if (++controlStores!=dropControlStore) C().ResidentNocWrite(offset,value); return; }
		assert(!current && !quarantined && C().reset==0x47800 && !hostDirty && value==1);
		assert(C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406);
		auto* c=C().command; bool write=c[7]==0x212; unsigned words=c[8]/4;
		assert(words && words<=4096 && !(words&31) && !c[6] && !c[9]);
		if (write) assert((c[0]==0x84000 || c[0]==0x88000) && !c[1] && c[2]==129
			&& c[3]==uint32_t(physical) && c[4]==uint32_t(physical>>32) && c[5]==(19|(24<<6)));
		else assert(c[0]==uint32_t(physical) && c[1]==uint32_t(physical>>32) && c[2]==(19|(24<<6))
			&& c[3]==0x144000 && !c[4] && c[5]==129 && !c[7]);
		transfers++; C().requests++; C().doorbells++; C().doorbellRead=false; C().pending=true; C().done=false;
		if (transfers==failTransfer) return;
		if (write && !dropPosted) for (unsigned i=0;i<words-(partialPosted?1:0);i++)
			OutputBufferMock::data[i]=C().buffers[4096+(c[0]-0x84000)/4+i]^(i==corruptPosted?1:0);
		if (!write) for (unsigned i=0;i<words;i++) data[4096+i]=OutputBufferMock::data[i]^(i==corruptReadback?1:0);
		unsigned at=write?4:0; nc[at]++; nc[at+1]+=words/16; nc[2]++; nc[6]=nc[2]; nc[at+3]+=1+(badTransferCounter?1:0);
		C().pending=false; C().done=true;
	}
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { CoordinatorIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && C().reset==0x87ffff && data[0]==0x31445243 && data[1]==2 && data[2]==1);
		assert(C().pciCommand[0]==0x402 && C().pciCommand[1]==0x402);
		for (unsigned w=0;w<TT_COORDINATOR_CODE_WORDS;w++) assert(program[w]==BlackholeCoordinatorReadbackCodeWord(w));
		C().reset=value; starts++; compareStarts++; if (executeTimeout) return;
		unsigned offset=data[3],words=data[4]; uint32_t mismatch=compareMismatch;
		for (unsigned i=0;i<words;i++) mismatch |= C().buffers[4096+offset+i]^data[4096+i];
		data[3072]=mismatch; data[3073]=offset; data[3074]=data[3075]=0;
		data[3076]=data[3077]=words; data[3078]=data[6]-(staleSummary?1:0); data[3079]=2;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; data[3088+i]=nc[i]; }
		if (badCompareCounter) nc[0]++;
		if (badSummary) data[3088]++;
		status[0]=TT_COORDINATOR_DONE; status[1]=words+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct OutputFixture {
	OutputIO io; PioneerResidentLease store; PioneerOutputBufferLease host;
	PioneerBulkUploadLease bulk; PioneerRuntimeMatvecLease compute; PioneerCoordinatorLease coordinator;
	tt_host_page_result input={}; tt_compute_noc_result noc={}; PioneerCoordinatedOutputResult result;
	std::vector<uint32_t> output=std::vector<uint32_t>(4096,0xdeadbeef);
	OutputFixture() {
		auto& c=*io.core[0]; assert(Prepared(c,input,store)); noc=c.Compute();
		c.bufferWords=TT_Q8_BULK_SAVED_BUFFER_WORDS;
		compute.ready=compute.saved=compute.bulk=compute.buffers_dirty=true; compute.buffer_words=TT_Q8_BULK_SAVED_BUFFER_WORDS;
		bulk.saved=bulk.ready=true; c.code[0]=0x0006b06f;
		assert(PioneerPrepareOutputBuffer(io,input,host));
		assert(PioneerCoordinatorPrepare(io,c.Baseline(),noc,store,coordinator,true,true));
		for (unsigned i=0;i<8192;i++) c.buffers[4096+i]=0x3f000000+i*17;
	}
	bool Run(unsigned words=4096,unsigned offset=0) {
		bool passed=PioneerCoordinatedOutput(io,io.core[0]->Baseline(),noc,input,store,host,bulk,compute,coordinator,offset,words,output.data(),result);
		if (passed) {
			assert(result.started && result.completed && result.stopped && result.compared && io.compareStarts==io.stops
				&& result.transfer.requests==2 && result.transfer.completed==2 && result.transfer.host_visible && result.transfer.readback_complete);
			for (unsigned i=0;i<words;i++) assert(output[i]==0x3f000000+(offset+i)*17);
			assert(!host.quarantined && !io.quarantined && !store.pending && !host.pending
				&& io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
		}
		return passed;
	}
};
int main()
{
	{ OutputFixture f; for (auto& value:f.io.nc) value=UINT32_MAX-1; assert(f.Run(4096) && f.Run(32,4096)); }
	for (unsigned words:{32U,64U,1024U,2048U,3072U,4096U}) for (unsigned offset:{0U,4096U}) {
		OutputFixture f; assert(f.Run(words,offset) && f.Run(words,offset) && f.coordinator.readbackNonce==2);
	}
	for (unsigned fault=0;fault<16;fault++) {
		OutputFixture f;
		f.io.dropPosted=fault==0; f.io.partialPosted=fault==1; f.io.corruptPosted=fault==2?4095:~0U;
		f.io.corruptReadback=fault==3?4095:~0U; f.io.lateHost=fault==4?4095:~0U;
		f.io.failTransfer=fault==5?1:fault==6?2:0; f.io.badTransferCounter=fault==7;
		f.io.badCompareCounter=fault==8; f.io.compareMismatch=fault==9; f.io.executeTimeout=fault==10;
		f.io.badPc=fault==11; f.io.badCount=fault==12; f.io.badMarker=fault==13;
		f.io.staleSummary=fault==14; f.io.badSummary=fault==15;
		assert(!f.Run() && f.result.transfer.recovery_required && f.bulk.unsafe && f.compute.unsafe
			&& f.store.recovery_required && f.host.quarantined && f.io.quarantined);
		if (fault==5 || fault==6 || fault==7 || fault>=10 || fault==8) assert(!f.io.stops);
	}
	for (unsigned word=0;word<32;word++) {
		OutputFixture f; f.io.OutputBufferMock::data[word]=0; f.io.dropHost=word+1;
		assert(!f.Run(32) && f.result.transfer.mismatches && !f.io.transfers && !f.io.compareStarts && !f.result.transfer.recovery_required);
	}
	for (unsigned word=1;word<=20;word++) {
		OutputFixture f; for (auto& value:f.io.C().command) value=0xdeadbeef; f.io.dropControlStore=word;
		bool passed=f.Run();
		if (word<=10) assert(!passed && !f.io.transfers && !f.io.compareStarts && !f.result.transfer.recovery_required);
		else if (!passed) assert(f.result.transfer.recovery_required && f.host.quarantined && !f.io.compareStarts);
	}
	for (unsigned fault=0;fault<4;fault++) {
		OutputFixture f; f.io.C().lateMapChange=fault==0; f.io.C().clearFails=fault==1;
		f.io.badLiveMap=fault==2; f.io.C().pciePending=fault==3;
		assert(!f.Run() && f.result.transfer.recovery_required && f.host.quarantined && f.io.quarantined && !f.io.stops);
	}
	for (unsigned field=0;field<9;field++) {
		OutputFixture f;
		if (field==0) f.compute.ready=false; if (field==1) f.compute.saved=false; if (field==2) f.compute.buffers_dirty=false;
		if (field==3) f.compute.running=true; if (field==4) f.coordinator.readback=false;
		if (field==5) f.coordinator.running=true; if (field==6) f.host.pending=true; if (field==7) f.bulk.unsafe=true;
		if (field==8) f.coordinator.readbackNonce=0x7ffffffe;
		assert(!f.Run() && !f.io.transfers && !f.io.compareStarts);
	}
	puts("PASS OFFLINE owned output DMA,finite poison replacement,frozen host snapshot,ordered full readback,all-bit device comparison,BME OFF during RISC,full source/PC/counter/nonce proof,unknown=>no reset+dual quarantine; NO native speed claim");
}
