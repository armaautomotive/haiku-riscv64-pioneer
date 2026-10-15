/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentStream.h"
struct StreamMock:StoreMock {
	unsigned payloadReads=0;
	bool dropHostPayload=false;
	uint32_t ResidentL1Read(unsigned w) { payloadReads++; return HostReadMock::ResidentL1Read(w); }
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset==0x40 && command[7]==0x212 && command[5]==(19|(24<<6))) {
			assert(!pending && !uncertain && value==1 && pciCommand[0]==0x406 && pciCommand[1]==0x406);
			assert(command[0]==0x17f000 && command[1]==0 && command[2]==(1|(2<<6)));
			assert(command[3]==uint32_t(physical) && command[4]==uint32_t(physical>>32) && command[6]==0 && command[8]==4096 && command[9]==0);
			doorbells++; doorbellRead=false;
			if (++Mock::writes==Mock::drop) { uncertain=true; return; }
			requests++; pending=true;
			if (failRequest==doorbells) { uncertain=true; return; }
			if (!dropHostPayload) for (unsigned w=0;w<1024;w++) page[w]=destination[1024+w];
			writeCounters[0]++; writeCounters[1]+=64; accepted++; writeCounters[3]++;
			pending=false; done=true; return;
		}
		StoreMock::ResidentNocWrite(offset,value);
	}
};
int main() {
	uint32_t data[1024]; for (unsigned w=0;w<1024;w++) data[w]=0x12340000+w*17;
	StreamMock io; tt_host_page_result host; PioneerResidentLease lease; tt_resident_stream_result r;
	assert(Prepared(io,host,lease)); auto baseline=io.Baseline(); auto noc=io.Compute();
	unsigned reads=io.payloadReads;
	assert(PioneerResidentStreamPage(io,baseline,noc,host,lease,12288,0,data,r));
	assert(r.requests==6 && r.completed==6 && r.host_write_issued && r.host_visible && r.readback_complete && !r.mismatches && !r.recovery_required);
	assert(io.payloadReads==reads); // no per-word PCIe payload read
	for (unsigned w=0;w<1024;w++) assert(io.device[w]==data[w] && io.page[w]==data[w] && io.destination[w]==data[w] && io.destination[1024+w]==data[w]);
	unsigned writes=io.Mock::writes;
	assert(PioneerResidentLeaseEnd(io,noc,lease)); assert(io.ReleaseHostPage()==0); io.Safe();
	for (unsigned drop=1;drop<=writes;drop++) {
		StreamMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); if (drop<=trial.Mock::writes) continue; trial.Mock::drop=drop;
		PioneerResidentStreamPage(trial,baseline,noc,h,l,12288,0,data,r);
		bool clean=PioneerResidentLeaseEnd(trial,noc,l);
		if (r.recovery_required || l.pending || l.recovery_required) assert(!clean);
		else { assert(clean && trial.ReleaseHostPage()==0); trial.Safe(); }
	}
	for (unsigned drop=1;drop<=2048;drop++) {
		StreamMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); trial.PageMock::drop=trial.PageMock::writes+drop;
		assert(!PioneerResidentStreamPage(trial,baseline,noc,h,l,12288,0,data,r));
		assert(!r.host_write_issued && !r.recovery_required && r.mismatches);
		assert(PioneerResidentLeaseEnd(trial,noc,l) && trial.ReleaseHostPage()==0); trial.Safe();
	}
	for (unsigned step=1;step<=6;step++) {
		StreamMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); trial.failRequest=step;
		assert(!PioneerResidentStreamPage(trial,baseline,noc,h,l,12288,0,data,r));
		assert(r.recovery_required && trial.quarantined && trial.requests==step && !trial.releases);
		assert(!PioneerResidentLeaseEnd(trial,noc,l));
	}
	for (unsigned fault=0;fault<7;fault++) {
		StreamMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l));
		trial.changed=fault==0; trial.lateMapChange=fault==1; trial.clearFails=fault==2;
		trial.badFlits=fault==3; trial.pciePending=fault==4; trial.corrupt=fault==5; trial.dropHostPayload=fault==6;
		assert(!PioneerResidentStreamPage(trial,baseline,noc,h,l,12288,0,data,r));
		assert(r.recovery_required && trial.quarantined && !PioneerResidentLeaseEnd(trial,noc,l));
		if (fault==5 || fault==6) assert(r.host_write_issued && !r.host_visible && !r.readback_complete && r.mismatches);
	}
}
