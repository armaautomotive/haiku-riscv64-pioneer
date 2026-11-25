/* SPDX-License-Identifier: MIT */
// Walk the real recorded model schedule; no model data or device is used.
#define main q35_activation_graph_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowActivationRead.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"
#include <cstring>

struct GraphReadPipeline {
    const DataflowCoreInvocation& invocation;
    unsigned issued=0,retired=0,bytes=0,maxPending=0;
    bool Start() { return true; }
    bool Issue(uint32_t sequence,uint32_t remote,uint32_t route,uint32_t local,unsigned count) {
        assert(sequence==issued && issued-retired<2 && local==DataflowActivationCacheAddress+bytes);
        const unsigned row=invocation.inputBase+bytes/sizeof(float);
        Range expected{};
        assert(!(count&63) && count<=DataflowActivationReadMaxBytes
            && invocation.input.Resolve(row,count/sizeof(float),0,count,expected)
            && expected.address==remote && expected.bytes==count
            && invocation.bankRoute[0][expected.bank]==route);
        bytes+=count;++issued;
        if (issued-retired>maxPending) maxPending=issued-retired;
        return true;
    }
    bool Complete(uint32_t sequence) { assert(sequence==retired && retired<issued);++retired;return true; }
    bool Finish() { return issued==retired && bytes==invocation.inputRows*sizeof(float); }
};
struct GraphSink {
    unsigned records=0,active=0,inactive=0,overlapped=0,typeMask=0;
    unsigned activeByCore[DataflowCoreCount]{},activeByStep[DataflowGraphDecodeSteps]{};
    bool projectionStep[DataflowGraphDecodeSteps]{};
    bool Begin(const DataflowStreamPlan&) { return true; }
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,const DataflowStreamRecord& record) {
        ++records;
        if (record.header[5]!=DataflowProgramMagic) return true;
        DataflowCoreInvocation invocation{};
        std::memcpy(&invocation,record.payload,sizeof(invocation));
        assert(step<DataflowGraphDecodeSteps && invocation.core==binding.core && invocation.Valid());
        projectionStep[step]=true;
        if (!invocation.assignment.active) { ++inactive;return true; }
        GraphReadPipeline pipeline{invocation};
        assert(DataflowActivationReadPipelined(invocation.input,invocation.inputBase,invocation.inputRows,
            DataflowActivationCacheAddress,invocation.bankRoute[0],pipeline));
        ++active;++activeByCore[invocation.core];++activeByStep[step];
        overlapped+=pipeline.maxPending==2;typeMask|=1U<<invocation.type;
        return true;
    }
    bool Seal(const DataflowStreamPlan&) { return true; }
    void Abort() { assert(false); }
};
int main() {
    (void)&q35_activation_graph_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto live=std::make_unique<DataflowProjectionAuthority>();
    auto after=std::make_unique<DataflowProjectionAuthority>();
    auto plan=std::make_unique<DataflowStreamPlan>();
    auto record=std::make_unique<DataflowStreamRecord>();
    auto projection=std::make_unique<DataflowProjectionProgram>();
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
    GraphSink sink;unsigned stage=0;
    assert(DataflowStreamRecording::Build(fixture->packed,*live,*after,*plan,fixture->routes,
        0,42,0,true,sink,*record,*projection,stage));
    assert(stage==DataflowDecoderSequenceComplete && sink.records==DataflowGraphDecodeSteps*DataflowCoreCount
        && sink.active && sink.overlapped && sink.typeMask);
    for (unsigned count:sink.activeByCore) assert(count);
    unsigned least=DataflowCoreCount,most=0,small=0,projectionSteps=0;
    for (unsigned step=0;step<DataflowGraphDecodeSteps;step++) if (sink.projectionStep[step]) {
        const unsigned count=sink.activeByStep[step];assert(count);
        ++projectionSteps;small+=count<=8;
        if (count<least) least=count;
        if (count>most) most=count;
    }
    std::printf("PASS graph activation plans: %u active, %u inactive, %u two-request; "
        "all 120 cores used, %u-%u per projection, %u/%u use at most 8; SIMULATED ONLY\n",
        sink.active,sink.inactive,sink.overlapped,least,most,small,projectionSteps);
}
