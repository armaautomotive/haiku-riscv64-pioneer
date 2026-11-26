/* SPDX-License-Identifier: MIT */
// Full recorder and dynamic position patch for the non-loadable hybrid graph.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_flash_hybrid_graph_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphProfile.h"
struct HybridSink {
    DataflowDynamicTokenContext dynamic{};
    DataflowDenseAttentionProgram prior[DataflowCoreCount]{};
    unsigned writes=0,producerSteps=0,reducerSteps=0,packSteps=0,priorStep=0,priorLayer=Layers;
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
        if (record.header[5]!=DataflowFlashHybridMagic) return true;
        DataflowFlashHybridInvocation invocation{};DataflowDynamicRead(record,invocation);
        assert(invocation.Valid() && invocation.core==binding.core && invocation.program.position==0);
        auto forged=invocation;forged.phase=0;assert(!forged.Valid());
        forged=invocation;forged.core=DataflowCoreCount;assert(!forged.Valid());
        DataflowFlashHybridPartition part{};assert(DataflowFlashHybridAssign(binding.core,0,part));
        assert(part.positionCount && part.blockCount);
        if (invocation.phase==DataflowFlashHybridProducer) {
            if (!binding.core) {priorStep=step;priorLayer=invocation.program.layer;++producerSteps;}
            assert(invocation.program.layer==priorLayer);prior[binding.core]=invocation.program;
        } else {
            assert(invocation.phase==DataflowFlashHybridReducer && step==priorStep+1
                && invocation.program.layer==priorLayer
                && !std::memcmp(&prior[binding.core],&invocation.program,sizeof(invocation.program)));
            if (!binding.core) ++reducerSteps;
        }
        auto patched=record;
        assert(DataflowDynamicPatch(patched,dynamic,binding.epoch,step+1,binding.core));
        DataflowFlashHybridInvocation rebound{};DataflowDynamicRead(patched,rebound);
        assert(rebound.Valid() && rebound.program.position==1 && rebound.phase==invocation.phase
            && rebound.core==invocation.core && rebound.program.layer==invocation.program.layer);
        assert(DataflowFlashHybridAssign(binding.core,1,part) && part.positionCount && part.blockCount);
        return true;
    }
    bool Seal(const DataflowStreamPlan&) {sealed=true;return true;}
    void Abort() {aborted=true;}
};
int main() {
    (void)&q35_flash_hybrid_graph_fixture_main;
    assert(DataflowStreamVersion==5 && DataflowGraphProfileOperation(DataflowFlashHybridMagic)==DataflowProfileDense);
    assert(DataflowGraphDecodeSteps==532 && DataflowGraphPromptSteps==529);
    auto fixture=std::make_unique<Fixture>();
    for (bool decode:{false,true}) {
        auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
        auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
        auto projection=std::make_unique<DataflowProjectionProgram>();auto sink=std::make_unique<HybridSink>();unsigned stage=0;
        assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
        const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
            fixture->routes,99,count,*plan));
        assert(DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
            0,42,0,decode,*sink,*record,*projection,stage));
        assert(sink->began && sink->sealed && !sink->aborted && sink->producerSteps==8
            && sink->reducerSteps==8 && sink->packSteps==16 && sink->writes==count*DataflowCoreCount);
    }
    std::puts("PASS prompt and decode record 8 paired hybrid dense steps for all 120 cores; SIMULATED ONLY");
}
