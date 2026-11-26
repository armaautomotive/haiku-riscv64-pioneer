/* SPDX-License-Identifier: MIT */
// Full non-loadable paired-hybrid graph with fused residual save and RMS.
#define PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY 1
#define PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_ffn_pair_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"
struct PairSink {
    const Fixture& fixture;unsigned writes=0,gate=0,up=0,pairs=0,fused=0,attention=0,post=0;
    bool sealed=false,aborted=false,failFused=false;
    DataflowDynamicTokenContext dynamic{};
    explicit PairSink(const Fixture& f):fixture(f) {}
    bool Begin(const DataflowStreamPlan&) {return true;}
    bool StageRecord(const DataflowStreamBinding& b,const Range&,unsigned step,const DataflowStreamRecord& record) {
        assert(b.core==writes%DataflowCoreCount && step==writes/DataflowCoreCount);
        assert(DataflowStreamRecordStructureValid(record,b.epoch,step+1,b.core));++writes;
        auto patched=record;
        assert(DataflowDynamicPatch(patched,dynamic,b.epoch,step+1,b.core));
        assert(DataflowStreamRecordStructureValid(patched,b.epoch,step+1,b.core));
        if (record.header[5]==DataflowRmsSaveMagic) {
            if (failFused && b.core==53) return false;
            DataflowRmsSaveInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid() && inv.core==b.core);
            assert(std::memcmp(&patched,&record,sizeof(record))==0);
            ++fused;if (inv.rms.role==AttentionNorm) ++attention;else if (inv.rms.role==PostAttentionNorm) ++post;else assert(false);
        }
        if (record.header[5]==DataflowResidualMagic) {
            DataflowResidualInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid() && inv.mode==DataflowResidualAdd);
        }
        if (record.header[5]==DataflowProgramMagic) {
            DataflowCoreInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid());
            if (inv.assignment.active) for (unsigned layer=0;layer<Layers;layer++) {
                if (DataflowRowSpaceMatches(inv.weights,fixture.manifest.layers[layer].weights[Gate].place)) ++gate;
                if (DataflowRowSpaceMatches(inv.weights,fixture.manifest.layers[layer].weights[Up].place)) ++up;
            }
        }
        if (b.core==DataflowCoreCount-1) {
            if (gate || up) {assert(gate==60 && up==60);++pairs;}
            gate=up=0;
        }
        return true;
    }
    bool Seal(const DataflowStreamPlan&) {sealed=true;return true;}
    void Abort() {aborted=true;}
};
int main() {
    (void)&q35_ffn_pair_fixture_main;
    static_assert(DataflowGraphDecodeSteps==436 && DataflowGraphPromptSteps==433,"fused save/RMS removes 64 graph steps");
    auto fixture=std::make_unique<Fixture>();
    for (bool decode:{false,true}) {
        auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
        auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
        auto projection=std::make_unique<DataflowProjectionProgram>();auto sink=std::make_unique<PairSink>(*fixture);unsigned stage=0;
        assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
        const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
            fixture->routes,99,count,*plan));
        assert(DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
            0,42,0,decode,*sink,*record,*projection,stage));
        assert(sink->sealed && !sink->aborted && sink->pairs==Layers && sink->fused==2*Layers*DataflowCoreCount
            && sink->attention==Layers*DataflowCoreCount && sink->post==Layers*DataflowCoreCount
            && sink->writes==count*DataflowCoreCount);
    }
    {
        auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
        auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
        auto projection=std::make_unique<DataflowProjectionProgram>();auto sink=std::make_unique<PairSink>(*fixture);unsigned stage=0;
        assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
            fixture->routes,99,DataflowGraphDecodeSteps,*plan));
        sink->failFused=true;
        assert(!DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
            0,42,0,true,*sink,*record,*projection,stage));
        assert(sink->aborted && !sink->sealed && sink->fused==53);
    }
    std::puts("PASS 64 fused residual/RMS and 32 paired FFN steps across 120 workers; SIMULATED ONLY");
}
