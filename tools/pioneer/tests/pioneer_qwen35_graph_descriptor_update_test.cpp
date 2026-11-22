/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define main q35_pci_fixture_main
#include "pioneer_qwen35_dataflow_pci_memory_test.cpp"
#undef main
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"

struct DescriptorRaw : MockPciRaw {
	int dropWrite=-1;
	void Q38PanelWrite(unsigned address,uint32_t value) {
		if (dropWrite>=0 && int(writes)==dropWrite) { writes++;return; }
		MockPciRaw::Q38PanelWrite(address,value);
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
	assert(firstWrites==128 && memory->UpdateGraphRun(0,1,next));
	assert(raw->writes-firstWrites==7);
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
	assert(!memory->UpdateGraphRun(0,2,third) && raw->writes==after+2);
	raw->dropWrite=-1;
	assert(!memory->UpdateGraphRun(0,2,third));
	puts("PASS graph descriptor update: sparse warm writes, immutable fields and partial-write retention; SIMULATED ONLY");
}
