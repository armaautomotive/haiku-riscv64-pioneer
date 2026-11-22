/* SPDX-License-Identifier: MIT */
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowGraphAuthorization.h"

int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	DataflowGraphRunTicket ticket{5,1001,545};DataflowGraphRunDescriptor scratch{};
	auto published=std::make_unique<PublisherIO>();DataflowGraphPublisher publisher;
	assert(publisher.Start(*plan,f->routes,token,ticket,5,3,0,*published,scratch)==DataflowGraphPublished);
	DataflowEmbeddingProgram embedding{};
	assert(DataflowEmbeddingProgramBuilder::Build(f->manifest,token.token,0,f->routes,embedding));
	alignas(64) uint32_t program[DataflowProgramRegionBytes/4]{};
	for (unsigned core:{0U,37U,119U}) {
		DataflowEmbeddingInvocation invocation{};DataflowStreamRecord record{};
		assert(DataflowEmbeddingProgramBuilder::BindCore(embedding,core,invocation));
		assert(DataflowStreamEncode(invocation,99,1,record));
		for (unsigned i=0;i<record.header[6]/4;i++) program[i]=record.payload[i];
		const auto& descriptor=published->records[core];
		assert(DataflowGraphRunValid(descriptor,core));
		assert(DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)));
		program[2]^=1;assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)));program[2]^=1;
		program[0]=0;assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)));program[0]=record.payload[0];
		record.payload[2]^=1;assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)));record.payload[2]^=1;
		record.header[3]=2;assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)));record.header[3]=1;
		assert(!DataflowGraphFetchedCommandMatches(descriptor,545,record,program,sizeof(program)));
		assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,nullptr,sizeof(program)));
		assert(!DataflowGraphFetchedCommandMatches(descriptor,0,record,program,sizeof(program)-4));
	}
	puts("PASS graph local command check: core, step, bounds, checksum and published payload; SIMULATED ONLY");
	return 0;
}
