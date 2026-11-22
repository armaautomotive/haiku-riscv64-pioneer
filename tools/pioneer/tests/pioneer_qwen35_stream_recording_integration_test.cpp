/* SPDX-License-Identifier: MIT */
// Reuse the complete synthetic model fixture and the real bank-store mock.
#define main q35_recorder_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#define main q35_bank_sink_fixture_main
#include "pioneer_qwen35_stream_bank_sink_test.cpp"
#undef main

int main() {
	(void)&q35_recorder_fixture_main;(void)&q35_bank_sink_fixture_main;
	auto f=std::make_unique<Fixture>();auto storage=std::make_unique<Storage>();
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++)
		f->routes[n][b]=storage->baseline.translated[b][0]|(storage->baseline.translated[b][1]<<6);
	auto live=std::make_unique<DataflowProjectionAuthority>();
	assert(live->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,111,548,*plan));
	auto candidate=std::make_unique<DataflowProjectionAuthority>();
	auto scratch=std::make_unique<DataflowStreamRecord>();auto projection=std::make_unique<DataflowProjectionProgram>();
	auto sink=std::make_unique<DataflowStreamPageSink<Storage>>(*storage);
	unsigned stage=~0U;
	assert(DataflowStreamRecording::Build(f->packed,*live,*candidate,*plan,f->routes,0,42,0,true,
		*sink,*scratch,*projection,stage));
	assert(stage==DataflowDecoderSequenceComplete && sink->Sealed()
		&& storage->writes==274*DataflowCoreCount && storage->raw.memory.size()==274*DataflowCoreCount);
	assert(!live->ActivationReady(0) && live->ConvNextPosition(0)==0 && candidate->ConvNextPosition(0)==1);
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto& lease=plan->lease[core];
		for (unsigned index:{0U,273U}) {
			uint32_t address=0,route=0,ledger=0;
			assert(storage->banks.Q35StreamPage(storage->baseline,sink.get(),lease.bank,
				lease.address-BankBase+index*4096,false,address,route,ledger));
			const auto it=storage->raw.memory.find(storage->raw.Key(route,address));
			assert(it!=storage->raw.memory.end());
			DataflowStreamRecord actual{};std::memcpy(&actual,it->second.data(),sizeof(actual));
			assert(DataflowStreamRecordStructureValid(actual,111,index*2+1,core));
			std::memcpy(&actual,it->second.data()+512,sizeof(actual));
			assert(DataflowStreamRecordStructureValid(actual,111,index*2+2,core));
		}
	}
	assert(PioneerResidentLeaseEnd(storage->raw,storage->noc,storage->lease));
	assert(storage->raw.ReleaseHostPage()==0);storage->raw.Safe();
	puts("PASS full graph recorder into protected page store: 548 steps, 120 cores; SIMULATED DEVICE");
}
