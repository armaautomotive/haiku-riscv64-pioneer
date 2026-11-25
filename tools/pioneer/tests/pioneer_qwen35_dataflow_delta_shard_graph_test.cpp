/* SPDX-License-Identifier: MIT */
// Record the actual fused graph with the experimental two-step delta schedule.
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_shard_source_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowGraphMetadata.h"
#include "../PioneerQwen35DataflowGraphProfile.h"
#include "../PioneerQwen35DataflowOwner.h"

struct ShardSink {
    unsigned writes=0,updates=0,normalizations=0,lastUpdate=0;
    bool began=false,sealed=false,aborted=false,patched=false;
    DataflowDynamicTokenContext next{};
    bool Begin(const DataflowStreamPlan&) { began=true;return true; }
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned index,const DataflowStreamRecord& record) {
        assert(binding.core==writes%DataflowCoreCount && index==writes/DataflowCoreCount);
        assert(DataflowStreamRecordStructureValid(record,binding.epoch,index+1,binding.core));
        ++writes;
        if (record.header[5]!=DataflowDeltaShardMagic) return true;
        DataflowDeltaShardInvocation invocation{};DataflowDynamicRead(record,invocation);
        assert(invocation.Valid() && invocation.core==binding.core && invocation.program.position==0);
        if (invocation.phase==DataflowDeltaShardUpdate) {
            if (!binding.core) lastUpdate=index;
            ++updates;
            assert(invocation.panelCount>0);
        } else {
            assert(invocation.phase==DataflowDeltaShardNormalize && index==lastUpdate+1);
            ++normalizations;
            assert((binding.core<DeltaHeads)==(invocation.panelCount!=0));
        }
        if (!patched && binding.core==0) {
            auto changed=record;
            assert(DataflowDynamicPatch(changed,next,binding.epoch,index+1,binding.core));
            DataflowDeltaShardInvocation rebound{};DataflowDynamicRead(changed,rebound);
            assert(rebound.Valid() && rebound.program.position==1
                && DataflowRowSpaceMatches(rebound.program.stateRead,invocation.program.stateWrite)
                && DataflowRowSpaceMatches(rebound.program.stateWrite,invocation.program.stateRead)
                && rebound.phase==invocation.phase);
            patched=true;
        }
        return true;
    }
    bool Seal(const DataflowStreamPlan&) { sealed=true;return true; }
    void Abort() { aborted=true; }
};
int main() {
    (void)&q35_shard_source_fixture_main;
    assert(DataflowGraphProfileOperation(DataflowDeltaShardMagic)==DataflowProfileDelta);
    struct NoLegacyIO {} noLegacyIO;
    DataflowProjectionOwner legacy;
    assert(legacy.RunDelta(DataflowDeltaProgram{},noLegacyIO)==DataflowOwnerInvalidState);
    auto f=std::make_unique<Fixture>();auto live=std::make_unique<DataflowProjectionAuthority>();
    assert(live->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
    for (bool decode:{false,true}) {
        auto plan=std::make_unique<DataflowStreamPlan>();
        const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
        assert(count==(decode?524U:521U));
        assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,count,*plan));
        auto candidate=std::make_unique<DataflowProjectionAuthority>();
        auto projection=std::make_unique<DataflowProjectionProgram>();
        auto record=std::make_unique<DataflowStreamRecord>();
        auto sink=std::make_unique<ShardSink>();
        assert(DataflowDynamicTokenContext::Build(f->manifest,42,1,sink->next));
        unsigned stage=~0U;
        assert(DataflowStreamRecording::Build(f->packed,*live,*candidate,*plan,f->routes,0,42,0,decode,
            *sink,*record,*projection,stage));
        assert(sink->began && sink->sealed && !sink->aborted && sink->patched
            && sink->writes==count*120 && sink->updates==24*120 && sink->normalizations==24*120);
        auto predicted=std::make_unique<DataflowProjectionAuthority>();
        assert(DataflowGraphMetadata::Prepare(*live,*predicted,0,42,0,decode,*projection,stage));
        assert(!live->ActivationReady(0) && candidate->ConvNextPosition(0)==1
            && predicted->ConvNextPosition(0)==1);
    }
    std::puts("PASS fused graph records 24 pairs of sharded delta phases with dynamic state slots; NO HARDWARE");
}
