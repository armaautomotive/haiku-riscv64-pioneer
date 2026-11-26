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
#include "../PioneerQwen35DataflowGraphMetadata.h"
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
    {
        auto live=std::make_unique<DataflowProjectionAuthority>();
        auto expected=std::make_unique<DataflowProjectionAuthority>();
        assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        DataflowEmbeddingProgram embed{};
        assert(live->BuildEmbedding(42,0,embed) && live->BeginEmbedding(embed) && live->CompleteEmbedding(embed));
        assert(live->CloneForGraphPlanning(*expected));
        DataflowResidualProgram save{};DataflowRmsProgram rms{};
        assert(live->BuildResidual(DataflowResidualSave,0,save));
        assert(live->BuildRms(0,AttentionNorm,0,1,rms));
        DataflowGraphAuthorityState before{},after{};
        assert(live->CaptureGraphState(before));
        auto wrong=save;wrong.activationSlot=1;
        DataflowGraphMetadataService rejected(*live);struct NoIO {} io;
        assert(rejected.RunRmsSave(rms,wrong,io)!=0 && rejected.Failed() && rejected.Steps()==0);
        assert(live->CaptureGraphState(after) && std::memcmp(&before,&after,sizeof(before))==0);
        assert(expected->BeginResidual(save) && expected->CompleteResidual(save)
            && expected->BeginRms(rms) && expected->CompleteRms(rms));
        DataflowGraphMetadataService metadata(*live);
        assert(metadata.RunRmsSave(rms,save,io)==0 && !metadata.Failed() && metadata.Steps()==1);
        assert(live->CaptureGraphState(after) && expected->GraphStateMatches(after));
    }
    for (bool decode:{false,true}) {
        auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
        auto predicted=std::make_unique<DataflowProjectionAuthority>();
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
        const bool metadataReady=DataflowGraphMetadata::Prepare(*live,*predicted,0,42,0,decode,*projection,stage);
        if (!metadataReady) std::fprintf(stderr,"fused metadata failed: decode=%u stage=%u\n",unsigned(decode),stage);
        assert(metadataReady);
        DataflowGraphAuthorityState recordedState{},predictedState{};
        assert(candidate->CaptureGraphState(recordedState) && predicted->CaptureGraphState(predictedState)
            && std::memcmp(&recordedState,&predictedState,sizeof(recordedState))==0);
        auto nextRecorded=std::make_unique<DataflowProjectionAuthority>();
        auto nextPredicted=std::make_unique<DataflowProjectionAuthority>();
        auto nextSink=std::make_unique<PairSink>(*fixture);
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,44,2,nextSink->dynamic));
        assert(DataflowStreamRecording::Build(fixture->packed,*candidate,*nextRecorded,*plan,fixture->routes,
            1,43,0,decode,*nextSink,*record,*projection,stage));
        assert(DataflowGraphMetadata::Prepare(*predicted,*nextPredicted,1,43,0,decode,*projection,stage));
        assert(nextRecorded->CaptureGraphState(recordedState) && nextPredicted->CaptureGraphState(predictedState)
            && std::memcmp(&recordedState,&predictedState,sizeof(recordedState))==0);
        assert(nextSink->sealed && !nextSink->aborted && nextSink->writes==count*DataflowCoreCount);
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
