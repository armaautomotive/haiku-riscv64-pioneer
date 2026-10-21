/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_BANKS_TEST_INCLUDED
#define PIONEER_RESIDENT_BANKS_TEST_INCLUDED
#define PIONEER_BULK_UPLOAD_MOCK_ONLY
#include "pioneer_bulk_upload_test.cpp"
#include <array>
#include <map>
static tt_dram_samples_result BankProof(StoreMock& io)
{
	auto result=io.Baseline();
	for (unsigned b=0;b<7;b++) for (unsigned s=0;s<3;s++) {
		uint32_t physical=BlackholeDramPhysicalX(b)|(BlackholeDramPhysicalY(b)<<6);
		result.id[b][s][0]=0x1c611000|physical;
		result.id[b][s][1]=0x80000|BlackholeDramBank(b);
		result.id[b][s][2]=0xe005;
		result.id[b][s][3]=physical;
	}
	return result;
}
struct BanksMock : BulkMock {
	BanksMock() { sevenBanks=true; }
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset!=0x40) { StoreMock::ResidentNocWrite(offset,value); return; }
		if (command[0]==TT_BULK_UPLOAD_L1 || command[3]==TT_BULK_UPLOAD_L1
			|| command[0]==TT_BULK_RETURN_L1 || command[3]==TT_BULK_RETURN_L1) {
			BulkMock::ResidentNocWrite(offset,value); return;
		}
		assert(!uncertain && !pending && value==1 && config==0x16005);
		doorbells++; doorbellRead=false;
		if (++Mock::writes==Mock::drop) { uncertain=true; return; }
		requests++; pending=true; done=false;
		if (failRequest==doorbells) { uncertain=true; return; }
		assert(!command[6] && command[8]==4096 && !command[9]);
		if (command[7]==0x212) {
			assert(command[0]==0x17e000 && !command[1] && command[2]==(1|(2<<6)) && !command[4]);
			assert(pciCommand[0]==0x402 && pciCommand[1]==0x402);
			auto& pageData=memory[Key(command[5],command[3])];
			for (unsigned w=0;w<1024;w++) pageData[w]=destination[w]^(corrupt?1:0);
			writeCounters[0]++; writeCounters[1]+=64+(badFlits?1:0); accepted++; writeCounters[3]++;
		} else {
			assert(!command[7] && !command[4] && command[5]==(1|(2<<6)));
			if (command[2]==(19|(24<<6))) {
				assert(command[0]==uint32_t(StoreMock::physical) && command[1]==uint32_t(StoreMock::physical>>32) && command[3]==0x17e000);
				assert(pciCommand[0]==0x406 && pciCommand[1]==0x406);
				for (unsigned w=0;w<1024;w++) destination[w]=page[w];
			} else {
				assert(!command[1] && command[3]==0x17f000);
				auto found=memory.find(Key(command[2],command[0])); assert(found!=memory.end());
				for (unsigned w=0;w<1024;w++) destination[1024+w]=found->second[w];
			}
			counters[0]++; counters[1]+=64+(badFlits?1:0); accepted++; counters[3]++;
		}
		pending=false; done=true;
	}
};
#ifndef PIONEER_BANKS_MOCK_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	static_assert(PioneerResidentBanks::Pages==5505024,"all-bank page bookkeeping");
	BanksMock io; auto baseline=BankProof(io); auto noc=io.Compute();
	PioneerResidentBanks banks;
	uint32_t address=0x123,route=0x456,index=0x789;
	assert(!banks.Page(baseline,0,0,address,route,index));
	assert(address==0x123 && route==0x456 && index==0x789);
	assert(banks.Bind(baseline) && !banks.Bind(baseline));
	auto copy=baseline; assert(!banks.Matches(copy)); // same bytes do not transfer ownership
	for (unsigned b=0;b<7;b++) {
		assert(BlackholeDramBank(b)!=3);
		for (uint64_t offset:{UINT64_C(0),PioneerResidentBanks::Capacity-4096}) {
			assert(banks.Page(baseline,b,offset,address,route,index));
			assert(address==0x04000000+offset && index==uint64_t(b)*786432+offset/4096);
		}
		assert(!banks.Range(baseline,b,PioneerResidentBanks::Capacity));
		assert(!banks.Range(baseline,b,PioneerResidentBanks::Capacity-4096,2));
		assert(banks.Range(baseline,b,PioneerResidentBanks::Capacity-16384,4));
	}
	for (unsigned b:{7U,UINT32_MAX}) assert(!banks.Range(baseline,b,0));
	for (uint64_t offset:{UINT64_C(1),UINT64_C(4095),UINT64_MAX,UINT64_MAX-4095}) assert(!banks.Range(baseline,0,offset));
	for (unsigned pages:{0U,5U,UINT32_MAX}) assert(!banks.Range(baseline,0,0,pages));
	// Every proof word must be validated before any hardware operation.
	for (unsigned b=0;b<7;b++) for (unsigned s=0;s<3;s++) for (unsigned w=0;w<4;w++) {
		auto bad=baseline; bad.id[b][s][w]^=w==2?0x1000:1;
		PioneerResidentBanks refused; assert(!refused.Bind(bad));
	}
	for (unsigned b=0;b<7;b++) for (unsigned xy=0;xy<2;xy++) {
		auto bad=baseline; bad.translated[b][xy]^=1;
		PioneerResidentBanks refused; assert(!refused.Bind(bad));
	}
	for (unsigned fault=0;fault<6;fault++) {
		auto bad=baseline;
		if (fault==0) bad.version=0;
		if (fault==1) bad.state=TT_DRAM_SAMPLE_BAD_ROUTE;
		if (fault==2) bad.routes_verified=6;
		if (fault==3) bad.tlb_restored=0;
		if (fault==4) bad.data_restored=0;
		if (fault==5) bad.recovery_required=1;
		PioneerResidentBanks refused; assert(!refused.Bind(bad));
	}
	assert(!io.Mock::writes && !io.requests);
	tt_host_page_result host; PioneerResidentLease lease; tt_resident_store_result r;
	assert(Prepared(io,host,lease));
	uint32_t data[1024],readback[1024];
	for (unsigned b=0;b<7;b++) for (uint64_t offset:{UINT64_C(0),PioneerResidentBanks::Capacity-4096}) {
		for (unsigned w=0;w<1024;w++) data[w]=0xface0000U^(b<<16)^uint32_t(offset)^w*17;
		assert(PioneerResidentStorePage(io,baseline,noc,host,lease,PioneerResidentBanks::Capacity,offset,true,data,r,&banks,b));
	}
	// Read ALL banks after ALL writes: different bank/page cannot alias.
	for (unsigned b=0;b<7;b++) for (uint64_t offset:{UINT64_C(0),PioneerResidentBanks::Capacity-4096}) {
		assert(PioneerResidentStorePage(io,baseline,noc,host,lease,PioneerResidentBanks::Capacity,offset,false,readback,r,&banks,b));
		for (unsigned w=0;w<1024;w++) assert(readback[w]==(0xface0000U^(b<<16)^uint32_t(offset)^w*17));
	}
	assert(io.seen==127 && io.memory.size()==14 && io.requests==56);
	unsigned requests=io.requests;
	for (unsigned b:{7U,UINT32_MAX}) assert(!PioneerResidentStorePage(io,baseline,noc,host,lease,PioneerResidentBanks::Capacity,0,true,data,r,&banks,b));
	assert(!PioneerResidentStorePage(io,baseline,noc,host,lease,4096,0,true,data,r,&banks,0));
	assert(!PioneerResidentStorePage(io,baseline,noc,host,lease,4096,0,true,data,r,nullptr,1));
	assert(io.requests==requests && !lease.recovery_required);
	assert(PioneerResidentLeaseEnd(io,noc,lease)); assert(io.ReleaseHostPage()==0); io.Safe();
	// Each of the three transfer stages on EVERY bank can become UNKNOWN.
	// No bank-local retry, restore, host free or later bank write is allowed.
	for (unsigned b=0;b<7;b++) for (unsigned step=1;step<=3;step++) {
		BanksMock trial; auto proof=BankProof(trial); PioneerResidentBanks owner;
		assert(owner.Bind(proof)); tt_host_page_result h; PioneerResidentLease l;
		assert(Prepared(trial,h,l)); trial.failRequest=step;
		assert(!PioneerResidentStorePage(trial,proof,noc,h,l,PioneerResidentBanks::Capacity,0,true,data,r,&owner,b));
		assert(r.recovery_required && trial.HostReadMock::quarantined && !trial.PageMock::releases);
		owner.Quarantine(); assert(owner.Quarantined() && !owner.Bind(proof));
		unsigned issued=trial.requests,writes=trial.Mock::writes;
		for (unsigned other=0;other<7;other++) assert(!PioneerResidentStorePage(trial,proof,noc,h,l,PioneerResidentBanks::Capacity,0,true,data,r,&owner,other));
		assert(trial.requests==issued && trial.Mock::writes==writes);
		assert(!PioneerResidentLeaseEnd(trial,noc,l) && !trial.PageMock::releases);
	}
	// A source change invalidates an already bound owner,not just new binds.
	baseline.source[1]^=1; assert(!banks.Matches(baseline));
	baseline.source[1]^=1; assert(banks.Matches(baseline));
	banks.Quarantine(); assert(!banks.Matches(baseline));
}
#endif
#endif
