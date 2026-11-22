/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQwen35DataflowStreamPageSink.h"
#include <memory>
#include <vector>
using namespace PioneerQwen35;

struct Storage {
	BanksMock raw;tt_dram_samples_result baseline=BankProof(raw);
	tt_compute_noc_result noc=raw.Compute();tt_host_page_result host{};PioneerResidentLease lease;
	PioneerResidentBanks banks;std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
	bool quarantined=false;unsigned writes=0;
	Storage() { assert(banks.Bind(baseline) && Prepared(raw,host,lease)); }
	bool Reserve(const DataflowStreamPlan& p,const void* owner) {
		PioneerQ35StreamBankRange ranges[DataflowCoreCount];
		for (unsigned i=0;i<DataflowCoreCount;i++) ranges[i]={p.lease[i].bank,p.lease[i].address-BankBase,p.lease[i].bytes};
		return banks.ReserveQ35Stream(baseline,owner,ranges,DataflowCoreCount,initialized.data(),initialized.size());
	}
	bool WriteVerifiedPage(const void* owner,unsigned bank,uint64_t offset,uint32_t* words) {
		writes++;tt_resident_store_result result{};
		return PioneerResidentStorePage(raw,baseline,noc,host,lease,PioneerResidentBanks::Capacity,
			offset,true,words,result,&banks,bank,owner);
	}
	bool Seal(const void* owner) { return banks.SealQ35Stream(owner); }
	void Quarantine() { quarantined=true;banks.Quarantine(); }
};
static DataflowStreamPlan Plan(unsigned count) {
	DataflowStreamPlan plan{};plan.ready=true;plan.epoch=17;plan.count=count;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const unsigned bank=core%Banks,offset=0x100000+(core/Banks)*8192;
		plan.lease[core]={bank,BankBase+offset,8192};
		plan.binding[core]={DataflowStreamBindingMagic,DataflowStreamVersion,17,core,bank,1+bank,BankBase+offset,count};
	}
	return plan;
}
static DataflowStreamRecord Record(unsigned core,unsigned step) {
	DataflowEmbeddingProgram p{};p.token=15;p.lane=15;p.weightBank=0;p.weightBase=BankBase;
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) p.bankRoute[n][b]=1+n*16+b;
	p.output.rows=Hidden;p.output.rowBytes=4;p.output.count=1;p.output.unit=16;
	p.output.parts[0]={1,0x20000000,0,Hidden};
	DataflowEmbeddingInvocation invocation{};DataflowStreamRecord record{};
	assert(DataflowEmbeddingProgramBuilder::BindCore(p,core,invocation)
		&& DataflowStreamEncode(invocation,17,step,record));return record;
}
int main() {
	(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
	const auto plan=Plan(3);
	Storage refused;auto overlap=plan;overlap.lease[7]=overlap.lease[0];
	assert(!refused.Reserve(overlap,&refused));
	const auto& claimed=plan.lease[0];
	const unsigned ledgerIndex=claimed.bank*(PioneerResidentBanks::Capacity/4096)+(claimed.address-BankBase)/4096;
	refused.initialized[ledgerIndex]=1;
	assert(!refused.Reserve(plan,&refused));refused.initialized[ledgerIndex]=0;
	assert(!refused.raw.requests);
	assert(PioneerResidentLeaseEnd(refused.raw,refused.noc,refused.lease));
	assert(refused.raw.ReleaseHostPage()==0);refused.raw.Safe();
	Storage storage;auto sink=std::make_unique<DataflowStreamPageSink<Storage>>(storage);
	assert(sink->Begin(plan));
	assert(!sink->Seal(plan));
	uint32_t address=0,route=0,index=0;
	assert(!storage.banks.Page(storage.baseline,plan.lease[0].bank,plan.lease[0].address-BankBase,address,route,index));
	assert(!storage.banks.Q35StreamPage(storage.baseline,&storage,plan.lease[0].bank,plan.lease[0].address-BankBase,true,address,route,index));
	for (unsigned step=0;step<3;step++) for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto record=Record(core,step+1);
		assert(sink->StageRecord(plan.binding[core],plan.lease[core],step,record));
	}
	assert(storage.writes==240 && sink->Seal(plan) && sink->Sealed() && !storage.quarantined);
	const auto& first=plan.lease[0];
	assert(!storage.banks.Q35StreamPage(storage.baseline,sink.get(),first.bank,first.address-BankBase,true,address,route,index));
	assert(storage.banks.Q35StreamPage(storage.baseline,sink.get(),first.bank,first.address-BankBase,false,address,route,index));
	uint32_t transfer[1024]{};tt_resident_store_result result{};
	const auto priorRequests=storage.raw.requests;
	assert(!PioneerResidentStorePage(storage.raw,storage.baseline,storage.noc,storage.host,storage.lease,
		PioneerResidentBanks::Capacity,first.address-BankBase,true,transfer,result,&storage.banks,first.bank));
	assert(!PioneerResidentStorePage(storage.raw,storage.baseline,storage.noc,storage.host,storage.lease,
		PioneerResidentBanks::Capacity,first.address-BankBase,true,transfer,result,&storage.banks,first.bank,sink.get()));
	assert(storage.raw.requests==priorRequests);
	assert(PioneerResidentStorePage(storage.raw,storage.baseline,storage.noc,storage.host,storage.lease,
		PioneerResidentBanks::Capacity,first.address-BankBase,false,transfer,result,&storage.banks,first.bank,sink.get()));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto& r=plan.lease[core];
		assert(storage.banks.Q35StreamPage(storage.baseline,sink.get(),r.bank,r.address-BankBase,false,address,route,index));
		const auto it=storage.raw.memory.find(storage.raw.Key(route,address));assert(it!=storage.raw.memory.end());
		const auto expected0=Record(core,1),expected1=Record(core,2);
		assert(std::memcmp(it->second.data(),&expected0,sizeof(expected0))==0
			&& std::memcmp(it->second.data()+512,&expected1,sizeof(expected1))==0);
		assert(storage.banks.Q35StreamPage(storage.baseline,sink.get(),r.bank,r.address-BankBase+4096,false,address,route,index));
		const auto last=storage.raw.memory.find(storage.raw.Key(route,address));assert(last!=storage.raw.memory.end());
		const auto expected2=Record(core,3);
		assert(std::memcmp(last->second.data(),&expected2,sizeof(expected2))==0);
		for (unsigned i=512;i<1024;i++) assert(last->second[i]==0);
	}
	assert(PioneerResidentLeaseEnd(storage.raw,storage.noc,storage.lease));
	assert(storage.raw.ReleaseHostPage()==0);storage.raw.Safe();
	Storage failed;auto broken=std::make_unique<DataflowStreamPageSink<Storage>>(failed);
	assert(broken->Begin(plan));failed.raw.failRequest=1;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto record=Record(core,1);assert(broken->StageRecord(plan.binding[core],plan.lease[core],0,record));
	}
	const auto record=Record(0,2);
	assert(!broken->StageRecord(plan.binding[0],plan.lease[0],1,record)
		&& broken->Unsafe() && failed.quarantined && failed.banks.Quarantined());
	puts("PASS protected stream pages: real bank resolver and DMA store with simulated hardware, odd records and uncertain transfer");
	return 0;
}
