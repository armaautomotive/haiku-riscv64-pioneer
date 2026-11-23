/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define main q35_pair_graph_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowGraphMetadata.h"
#include "../PioneerQwen35DataflowDynamicToken.h"
#include <cstring>
struct PairGraphSink {
    Fixture& fixture;
    DataflowDynamicTokenContext dynamic{};
    unsigned writes=0,pairs=0,active=0,alphaCount[Layers]{},betaCount[Layers]{};bool aborted=false;
    int failAt=-1;
    explicit PairGraphSink(Fixture& f):fixture(f) {
        assert(DataflowDynamicTokenContext::Build(fixture.manifest,42,1,dynamic));
    }
    bool Begin(const DataflowStreamPlan&) {return true;}
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,
        const DataflowStreamRecord& record) {
        if (int(writes)==failAt) return false;
        assert(binding.core==writes%DataflowCoreCount && step==writes/DataflowCoreCount);
        assert(DataflowStreamRecordStructureValid(record,binding.epoch,step+1,binding.core));
        if (!binding.core) {active=0;std::memset(alphaCount,0,sizeof(alphaCount));std::memset(betaCount,0,sizeof(betaCount));}
        if (record.header[5]==DataflowProgramMagic) {
            DataflowCoreInvocation invocation{};std::memcpy(&invocation,record.payload,sizeof(invocation));
            if (invocation.assignment.active) {
                active++;
                for (unsigned layer=0;layer<Layers;layer++) {
                    if (Dense(layer)) continue;
                    const bool isAlpha=DataflowRowSpaceMatches(invocation.weights,fixture.manifest.layers[layer].weights[Alpha].place);
                    const bool isBeta=DataflowRowSpaceMatches(invocation.weights,fixture.manifest.layers[layer].weights[Beta].place);
                    if (!isAlpha && !isBeta) continue;
                    assert(isAlpha!=isBeta);
                    assert(DataflowRowSpaceMatches(invocation.output,fixture.workspace.space[isAlpha
                        ?DataflowGraphRecurrentAlphaWorkspaceIndex:DataflowGraphRecurrentBetaWorkspaceIndex]));
                    if (isAlpha) alphaCount[layer]++;else betaCount[layer]++;
                    auto patched=record;
                    assert(DataflowDynamicPatch(patched,dynamic,binding.epoch,step+1,binding.core));
                    assert(DataflowStreamRecordStructureValid(patched,binding.epoch,step+1,binding.core));
                }
            }
        }
        if (binding.core==DataflowCoreCount-1) for (unsigned layer=0;layer<Layers;layer++) {
            if (!alphaCount[layer] && !betaCount[layer]) continue;
            assert(alphaCount[layer]==2 && betaCount[layer]==2 && active==4);
            pairs++;
        }
        writes++;return true;
    }
    bool Seal(const DataflowStreamPlan& plan) {return writes==plan.count*DataflowCoreCount;}
    void Abort() {aborted=true;}
};
struct SequentialMetadata : DataflowGraphMetadataService {
    explicit SequentialMetadata(DataflowProjectionAuthority& authority):DataflowGraphMetadataService(authority) {}
    void RunIndependentProjectionPair()=delete;
};
int main() {
    (void)&q35_pair_graph_fixture_main;
    static_assert(DataflowGraphDecodeSteps==524 && DataflowGraphPromptSteps==521,"paired graph sizes");
    auto fixture=std::make_unique<Fixture>();
    auto live=std::make_unique<DataflowProjectionAuthority>();
    auto recorded=std::make_unique<DataflowProjectionAuthority>();
    auto replay=std::make_unique<DataflowProjectionAuthority>();
    auto plan=std::make_unique<DataflowStreamPlan>();
    auto scratch=std::make_unique<DataflowStreamRecord>();
    auto projection=std::make_unique<DataflowProjectionProgram>();
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    for (bool decode:{true,false}) {
        recorded=std::make_unique<DataflowProjectionAuthority>();
        replay=std::make_unique<DataflowProjectionAuthority>();
        const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
            fixture->rope,fixture->routes,19,count,*plan));
        PairGraphSink sink(*fixture);unsigned stage=0;
        assert(DataflowStreamRecording::Build(fixture->packed,*live,*recorded,*plan,fixture->routes,
            0,42,0,decode,sink,*scratch,*projection,stage));
        if (sink.pairs!=24 || sink.writes!=count*DataflowCoreCount || sink.aborted)
            std::fprintf(stderr,"paired graph pairs=%u writes=%u expected=%u aborted=%d\n",sink.pairs,sink.writes,count*DataflowCoreCount,int(sink.aborted));
        assert(sink.pairs==24 && sink.writes==count*DataflowCoreCount && !sink.aborted);
        assert(DataflowGraphMetadata::Prepare(*live,*replay,0,42,0,decode,*projection,stage));
        auto sequential=std::make_unique<DataflowProjectionAuthority>();
        assert(live->CloneForGraphPlanning(*sequential));
        SequentialMetadata sequentialService(*sequential);
        struct NoIO {} noIO;unsigned ignored=Vocabulary;
        assert(DataflowDecoderSequence::Run(0,42,0,*sequential,sequentialService,noIO,
            ignored,stage,*projection,decode)==DataflowDecoderSequenceComplete);
        assert(sequentialService.Steps()==(decode?548U:545U));
        DataflowGraphAuthorityState expected{};
        assert(sequential->CaptureGraphState(expected) && recorded->GraphStateMatches(expected)
            && replay->GraphStateMatches(expected));
        for (unsigned layer=0;layer<Layers;layer++) if (!Dense(layer)) {
            assert(recorded->ConvNextPosition(layer)==1 && recorded->DeltaNextPosition(layer)==1);
            assert(replay->ConvNextPosition(layer)==1 && replay->DeltaNextPosition(layer)==1);
            assert(live->ConvNextPosition(layer)==0 && live->DeltaNextPosition(layer)==0);
        }
    }
    recorded=std::make_unique<DataflowProjectionAuthority>();
    PairGraphSink failed(*fixture);failed.failAt=3;unsigned stage=0;
    assert(!DataflowStreamRecording::Build(fixture->packed,*live,*recorded,*plan,fixture->routes,
        0,42,0,false,failed,*scratch,*projection,stage));
    assert(failed.aborted && !live->Pending());
    std::puts("PASS paired complete decode/prompt graph: 24 paired steps, metadata replay and partial-write abort; SIMULATED ONLY");
}
