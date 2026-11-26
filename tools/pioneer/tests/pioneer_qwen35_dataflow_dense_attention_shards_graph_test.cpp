/* SPDX-License-Identifier: MIT */
// Real full-graph recorder with a simulated model layout; no model or card.
#define PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_dense_shard_graph_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphProfile.h"
struct ShardSink {
    DataflowDynamicTokenContext dynamic{};
    unsigned writes=0,shards=0,packSteps=0,coverage[DenseHeads*DenseHeadWidth]{};
    bool began=false,sealed=false,aborted=false;
    bool Begin(const DataflowStreamPlan&) {began=true;return true;}
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,const DataflowStreamRecord& record) {
        assert(binding.core==writes%DataflowCoreCount && step==writes/DataflowCoreCount);
        assert(DataflowStreamRecordStructureValid(record,binding.epoch,step+1,binding.core));++writes;
        if (record.header[5]==DataflowDenseAttentionMagic) {
            DataflowDenseAttentionInvocation invocation{};DataflowDynamicRead(record,invocation);
            assert(invocation.Valid() && invocation.program.mode!=DataflowDenseAttentionRun);
            if (!binding.core) ++packSteps;
        }
        if (record.header[5]!=DataflowDenseAttentionShardMagic) return true;
        DataflowDenseAttentionShardInvocation invocation{};DataflowDynamicRead(record,invocation);
        assert(invocation.Valid() && invocation.core==binding.core && invocation.program.position==0);
        auto forged=invocation;forged.firstLane^=16;assert(!forged.Valid());
        forged=invocation;forged.head=(forged.head+1)%DenseHeads;assert(!forged.Valid());
        if (!binding.core) std::memset(coverage,0,sizeof(coverage));
        for (unsigned lane=invocation.firstLane;lane<invocation.firstLane+invocation.laneCount;lane++)
            assert(++coverage[invocation.head*DenseHeadWidth+lane]==1);
        auto patched=record;
        assert(DataflowDynamicPatch(patched,dynamic,binding.epoch,step+1,binding.core));
        DataflowDenseAttentionShardInvocation rebound{};DataflowDynamicRead(patched,rebound);
        assert(rebound.Valid() && rebound.program.position==1 && rebound.head==invocation.head
            && rebound.firstLane==invocation.firstLane && rebound.laneCount==invocation.laneCount);
        if (binding.core==DataflowCoreCount-1) {
            for (unsigned count:coverage) assert(count==1);
            ++shards;
        }
        return true;
    }
    bool Seal(const DataflowStreamPlan&) {sealed=true;return true;}
    void Abort() {aborted=true;}
};
int main() {
    (void)&q35_dense_shard_graph_fixture_main;
    assert(DataflowStreamVersion==4 && DataflowGraphProfileOperation(DataflowDenseAttentionShardMagic)==DataflowProfileDense);
    auto fixture=std::make_unique<Fixture>();auto live=std::make_unique<DataflowProjectionAuthority>();
    auto candidate=std::make_unique<DataflowProjectionAuthority>();auto plan=std::make_unique<DataflowStreamPlan>();
    auto record=std::make_unique<DataflowStreamRecord>();auto projection=std::make_unique<DataflowProjectionProgram>();
    auto sink=std::make_unique<ShardSink>();unsigned stage=0;
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
        fixture->routes,99,DataflowGraphDecodeSteps,*plan));
    assert(DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
        0,42,0,true,*sink,*record,*projection,stage));
    assert(sink->began && sink->sealed && !sink->aborted && sink->shards==8 && sink->packSteps==16
        && sink->writes==DataflowGraphDecodeSteps*DataflowCoreCount);
    std::puts("PASS 8 recorded dense attention steps distribute output across 120 cores and patch position; SIMULATED ONLY");
}
