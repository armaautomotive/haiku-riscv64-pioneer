/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_READ_MOCK_ONLY
#include "pioneer_host_read_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentStore.h"
struct StoreMock : HostReadMock {
	uint32_t device[3072]={},writeCounters[4]={100,200,30,400};
	unsigned accepted=30,doorbells=0,failRequest=0;
	bool uncertain=false,pending=false,corrupt=false,restoreTlbFails=false;
	StoreMock() { transferBytes=8192; for (unsigned w=0;w<3072;w++) device[w]=0xabcd0000+w; }
	void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
		if (restoreTlbFails && value==1+slot*3+word) return;
		Mock::DramTlbWrite(slot,word,value);
	}
	void ResidentL1Write(unsigned w,uint32_t value) {
		assert(!uncertain); HostReadMock::ResidentL1Write(w,value);
	}
	void HostPciWrite(unsigned peer,uint16_t offset,uint8_t size,uint32_t value) {
		assert(peer<2 && offset==4 && size==2 && (value==0x402 || value==0x406));
		if (value==0x406) { assert(!pending && !uncertain && PageMock::allocated && !cached); bmeWrites++; }
		if (++Mock::writes==Mock::drop || (clearFails && bmeWrites && value==0x402)) return;
		pciCommand[peer]=value;
	}
	uint32_t ResidentNocRead(uint32_t offset) {
		assert(BlackholeHostWriteReadRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (offset==0x40) { if (doorbells) { doorbellRead=true; orderedReads++; } return pending?1:0; }
		if (offset==0x204 || offset==0x220 || offset==0x228) {
			if (doorbells) assert(doorbellRead);
			return writeCounters[offset==0x204?0:offset==0x220?1:3];
		}
		if (offset==0x210) { if (doorbells) assert(doorbellRead); return accepted; }
		return Mock::ResidentNocRead(offset);
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		assert(!uncertain && BlackholeResidentWriteRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (offset!=0x40) {
			assert(!pending);
			if (++Mock::writes==Mock::drop) return;
			if (offset<=0x24) command[offset/4]=value; else { assert(offset==0x100); config=value; }
			return;
		}
		assert(value==1 && config==0x16005 && !pending);
		doorbells++; doorbellRead=false;
		if (++Mock::writes==Mock::drop) { uncertain=true; return; }
		requests++; pending=true; done=false;
		if (failRequest==doorbells) { uncertain=true; return; }
		assert(command[6]==0 && command[8]==4096 && command[9]==0);
		if (command[7]==0x212) {
			assert(command[0]==0x17e000 && command[1]==0 && command[2]==(1|(2<<6)) && command[4]==0 && command[5]==(18|(12<<6)));
			assert(pciCommand[0]==0x402 && pciCommand[1]==0x402);
			unsigned index=(command[3]-0x04000000)/4; assert(index+1024<=3072);
			for (unsigned w=0;w<1024;w++) device[index+w]=destination[w]^(corrupt?1:0);
			writeCounters[0]++; writeCounters[1]+=64+(badFlits?1:0); accepted++; writeCounters[3]++;
		} else {
			assert(command[7]==0 && command[4]==0 && command[5]==(1|(2<<6)));
			if (command[2]==(19|(24<<6))) {
				assert(command[0]==uint32_t(physical) && command[1]==uint32_t(physical>>32) && command[3]==0x17e000);
				assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
				for (unsigned w=0;w<1024;w++) destination[w]=page[w];
			} else {
				assert(command[2]==(18|(12<<6)) && command[1]==0 && command[3]==0x17f000);
				unsigned index=(command[0]-0x04000000)/4; assert(index+1024<=3072);
				for (unsigned w=0;w<1024;w++) destination[1024+w]=device[index+w];
			}
			counters[0]++; counters[1]+=64+(badFlits?1:0); accepted++; counters[3]++;
		}
		pending=false; done=true;
	}
};
static bool Prepared(StoreMock& io,tt_host_page_result& host,PioneerResidentLease& lease)
{
	return PioneerPrepareHostPage(io,host) && PioneerResidentLeaseBegin(io,io.Compute(),lease);
}
int main()
{
	uint32_t first[1024],second[1024],readback[1024]={};
	for (unsigned w=0;w<1024;w++) { first[w]=0x12340000+w*17; second[w]=~first[w]; }
	StoreMock io; tt_host_page_result host; PioneerResidentLease lease; tt_resident_store_result r;
	assert(Prepared(io,host,lease)); auto baseline=io.Baseline(); auto noc=io.Compute();
	assert(PioneerResidentStorePage(io,baseline,noc,host,lease,12288,0,true,first,r));
	assert(PioneerResidentStorePage(io,baseline,noc,host,lease,12288,8192,true,second,r));
	assert(PioneerResidentStorePage(io,baseline,noc,host,lease,12288,0,false,readback,r));
	assert(!memcmp(first,readback,4096));
	assert(PioneerResidentStorePage(io,baseline,noc,host,lease,12288,8192,false,readback,r));
	assert(!memcmp(second,readback,4096));
	for (unsigned w=0;w<1024;w++) assert(io.device[1024+w]==0xabcd0000+1024+w);
	assert(PioneerResidentLeaseEnd(io,noc,lease)); assert(io.ReleaseHostPage()==0); io.Safe();
	unsigned wholeWrites=0;
	{
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l));
		assert(PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r));
		assert(PioneerResidentLeaseEnd(trial,noc,l)); wholeWrites=trial.Mock::writes;
	}
	for (unsigned drop=1;drop<=wholeWrites;drop++) {
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		trial.Mock::drop=drop;
		bool prepared=Prepared(trial,h,l);
		if (prepared) PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r);
		bool clean=PioneerResidentLeaseEnd(trial,noc,l);
		assert(trial.requests<=3);
		if (l.pending || l.recovery_required) assert(!clean);
		if (clean) { assert(trial.ReleaseHostPage()==0); trial.Safe(); }
	}
	for (unsigned drop=1;drop<=3072;drop++) {
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		trial.PageMock::drop=drop;
		bool prepared=Prepared(trial,h,l);
		if (prepared) assert(!PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r));
		assert(!trial.requests && !trial.bmeWrites && !l.recovery_required);
		assert(PioneerResidentLeaseEnd(trial,noc,l)); assert(trial.ReleaseHostPage()==0); trial.Safe();
	}
	for (unsigned step=1;step<=3;step++) {
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); trial.failRequest=step;
		assert(!PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r));
		assert(r.recovery_required && trial.quarantined && !trial.releases);
		assert(!PioneerResidentLeaseEnd(trial,noc,l));
	}
	for (unsigned fault=0;fault<5;fault++) {
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l));
		trial.changed=fault==0; trial.lateMapChange=fault==1; trial.clearFails=fault==2;
		trial.badFlits=fault==3; trial.pciePending=fault==4;
		assert(!PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r));
		assert(r.recovery_required && trial.quarantined && trial.requests==1);
		assert(!PioneerResidentLeaseEnd(trial,noc,l));
	}
	{
		StoreMock trial; tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); unsigned requests=trial.requests;
		assert(!PioneerResidentStorePage(trial,baseline,noc,h,l,12288,UINT64_MAX-4095,true,first,r));
		assert(trial.requests==requests && !l.recovery_required);
		trial.corrupt=true;
		assert(!PioneerResidentStorePage(trial,baseline,noc,h,l,12288,0,true,first,r));
		assert(r.state==TT_STORE_MISMATCH && !r.recovery_required && r.mismatches==1024);
		assert(PioneerResidentLeaseEnd(trial,noc,l)); assert(trial.ReleaseHostPage()==0); trial.Safe();
	}
}
