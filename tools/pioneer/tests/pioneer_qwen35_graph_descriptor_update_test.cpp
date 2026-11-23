/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define main q35_pci_fixture_main
#include "pioneer_qwen35_dataflow_pci_memory_test.cpp"
#undef main
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowStageLedger.h"

struct DescriptorRaw : MockPciRaw {
	int dropWrite=-1;
	mutable unsigned reads=0;
	uint32_t Q38PanelRead(unsigned address) const {
		++reads;return MockPciRaw::Q38PanelRead(address);
	}
	void Q38PanelWrite(unsigned address,uint32_t value) {
		if (dropWrite>=0 && int(writes)==dropWrite) { writes++;return; }
		MockPciRaw::Q38PanelWrite(address,value);
	}
};

struct FleetDescriptorRaw : DescriptorRaw {
	uint32_t descriptor[DataflowCoreCount][DataflowGraphRunBytes/4]{};
	uint32_t Q38PanelRead(unsigned address) const {
		if (address>=DataflowGraphRunAddress && address<DataflowGraphRunAddress+DataflowGraphRunBytes) {
			assert(selected<DataflowCoreCount);++reads;
			return descriptor[selected][(address-DataflowGraphRunAddress)/4];
		}
		return DescriptorRaw::Q38PanelRead(address);
	}
	void Q38PanelWrite(unsigned address,uint32_t value) {
		if (address>=DataflowGraphRunAddress && address<DataflowGraphRunAddress+DataflowGraphRunBytes) {
			assert(selected<DataflowCoreCount);
			descriptor[selected][(address-DataflowGraphRunAddress)/4]=value;++writes;return;
		}
		DescriptorRaw::Q38PanelWrite(address,value);
	}
};

int main() {
	(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext firstToken{},nextToken{};
	assert(DataflowDynamicTokenContext::Build(f->manifest,11,0,firstToken));
	assert(DataflowDynamicTokenContext::Build(f->manifest,12,1,nextToken));
	DataflowGraphRunDescriptor first{},next{};
	assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,firstToken,
		{5,1,545},1,100,0,first));
	assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,nextToken,
		{6,546,548},2,100,0,next));
	auto raw=std::make_unique<DescriptorRaw>();
	auto memory=std::make_unique<PioneerQwen35DataflowPciMemory<DescriptorRaw>>(*raw);
	raw->reset[0]=DataflowPciRunningReset;
	assert(memory->WriteCoreMemory(0,DataflowGraphRunAddress,&first,sizeof(first)));
	const unsigned firstWrites=raw->writes;
	const unsigned positionAddress=DataflowGraphRunAddress+offsetof(DataflowGraphRunDescriptor,token)
		+offsetof(DataflowDynamicTokenContext,position);
	raw->Q38PanelWrite(positionAddress,99);
	assert(!memory->UpdateGraphRun(0,1,next,&first)); // intervening mutable-word change
	raw->Q38PanelWrite(positionAddress,first.token.position);
	const unsigned beforeReads=raw->reads,beforeWrites=raw->writes;
	assert(firstWrites==128 && memory->UpdateGraphRun(0,1,next,&first));
	assert(raw->reads-beforeReads==15); // eight old mutable words and seven write readbacks
	assert(raw->writes-beforeWrites==7);
	DataflowGraphRunDescriptor actual{};
	assert(memory->ReadCoreMemory(0,DataflowGraphRunAddress,&actual,sizeof(actual))
		&& !std::memcmp(&actual,&next,sizeof(next)));
	const unsigned after=raw->writes;
	assert(!memory->UpdateGraphRun(0,1,next) && raw->writes==after);
	auto invalidStatic=next;invalidStatic.triggerSequence=3;invalidStatic.binding.epoch++;
	invalidStatic.checksum=DataflowGraphRunChecksum(invalidStatic);
	assert(DataflowGraphRunValid(invalidStatic,0));
	assert(!memory->UpdateGraphRun(0,2,invalidStatic) && raw->writes==after);
	DataflowGraphRunDescriptor third=next;third.triggerSequence=3;third.ticket.epoch++;
	third.ticket.firstSequence+=third.ticket.count;third.token.position++;
	third.checksum=DataflowGraphRunChecksum(third);assert(DataflowGraphRunValid(third,0));
	raw->idleForWrite=false;
	assert(!memory->UpdateGraphRun(0,2,third) && raw->writes==after);
	raw->idleForWrite=true;raw->dropWrite=int(raw->writes)+1;
	assert(!memory->UpdateGraphRun(0,2,third,&next) && raw->writes==after+2);
	raw->dropWrite=-1;
	assert(!memory->UpdateGraphRun(0,2,third));
	// If an immutable word changes after its audit, the device's existing
	// full descriptor/checksum validation rejects the next command.
	auto other=std::make_unique<DescriptorRaw>();
	auto otherMemory=std::make_unique<PioneerQwen35DataflowPciMemory<DescriptorRaw>>(*other);
	other->reset[0]=DataflowPciRunningReset;
	assert(otherMemory->WriteCoreMemory(0,DataflowGraphRunAddress,&first,sizeof(first)));
	const unsigned staticAddress=DataflowGraphRunAddress+offsetof(DataflowGraphRunDescriptor,binding);
	const uint32_t original=other->Q38PanelRead(staticAddress);
	other->Q38PanelWrite(staticAddress,original^1U);
	assert(otherMemory->UpdateGraphRun(0,1,next,&first));
	assert(otherMemory->ReadCoreMemory(0,DataflowGraphRunAddress,&actual,sizeof(actual))
		&& !DataflowGraphRunValid(actual,0));
	// Without an audit receipt the conservative full-read path still rejects
	// the same corruption before any descriptor update is issued.
	assert(otherMemory->WriteCoreMemory(0,DataflowGraphRunAddress,&first,sizeof(first)));
	other->Q38PanelWrite(staticAddress,original^1U);
	const unsigned corruptWrites=other->writes;
	assert(!otherMemory->UpdateGraphRun(0,1,next) && other->writes==corruptWrites);
	// Compose the real ledger and PCI update over a simulated complete fleet:
	// first publish, completion readback/audit, then one warm publication.
	auto fleet=std::make_unique<FleetDescriptorRaw>();
	auto fleetMemory=std::make_unique<PioneerQwen35DataflowPciMemory<FleetDescriptorRaw>>(*fleet);
	auto ledger=std::make_unique<DataflowGraphStageLedger>();
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		fleet->reset[core]=DataflowPciRunningReset;
		DataflowGraphRunDescriptor d{};
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,firstToken,
			{5,1,545},1,100,0,d));
		assert(ledger->CanStage(core,d) && fleetMemory->WriteCoreMemory(core,DataflowGraphRunAddress,&d,sizeof(d))
			&& ledger->Record(core,d) && ledger->Verify(core,d));
	}
	for (unsigned core=0;core<DataflowCoreCount;core++) assert(ledger->BeginPublish(core,1));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowGraphRunDescriptor expected{},observed{};
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,firstToken,
			{5,1,545},1,100,0,expected));
		assert(fleetMemory->ReadCoreMemory(core,DataflowGraphRunAddress,&observed,sizeof(observed))
			&& !std::memcmp(&expected,&observed,sizeof(expected)) && ledger->RecordAudit(core,expected));
	}
	const unsigned fleetReads=fleet->reads,fleetWrites=fleet->writes;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowGraphRunDescriptor d{};
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,nextToken,
			{6,546,548},2,100,0,d) && ledger->CanStage(core,d));
		const auto* old=ledger->TakeAudited(core,1);
		assert(old && fleetMemory->UpdateGraphRun(core,1,d,old)
			&& ledger->Record(core,d) && ledger->Verify(core,d));
	}
	assert(fleet->reads-fleetReads==15*DataflowCoreCount
		&& fleet->writes-fleetWrites==7*DataflowCoreCount);
	for (unsigned core=0;core<DataflowCoreCount;core++) assert(ledger->BeginPublish(core,2));
	puts("PASS graph descriptor update: sparse warm writes, immutable fields and partial-write retention; SIMULATED ONLY");
}
