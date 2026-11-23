/* SPDX-License-Identifier: MIT */
#define main q35_pair_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include <cstring>
#include "../PioneerQwen35DataflowDynamicToken.h"
struct PairSink {
    const DataflowProjectionProgram& a;const DataflowProjectionProgram& b;
    unsigned count=0,first=0,second=0,inactive=0;
    int failAt=-1;
    const DataflowDynamicTokenContext* dynamic=nullptr;
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned index,
        const DataflowStreamRecord& record) {
        if (int(count)==failAt) return false;
        assert(index==0 && binding.core==count);
        assert(DataflowStreamRecordStructureValid(record,binding.epoch,1,count));
        if (dynamic) {
            DataflowStreamRecord patched=record;
            assert(DataflowDynamicPatch(patched,*dynamic,binding.epoch,1,count));
            assert(DataflowStreamRecordStructureValid(patched,binding.epoch,1,count));
        }
        DataflowCoreInvocation actual{};
        std::memcpy(&actual,record.payload,sizeof(actual));
        assert(actual.Valid() && actual.core==count);
        DataflowCoreInvocation expected{};
        if (a.core[count].active) {
            assert(DataflowProgramBuilder::BindCore(a,count,expected));first++;
        } else {
            unsigned rank=0;
            for (unsigned i=0;i<count;i++) if (!a.core[i].active) rank++;
            unsigned source=0;
            for (;source<DataflowCoreCount;source++) if (b.core[source].active) {
                if (!rank) break;
                rank--;
            }
            if (source<DataflowCoreCount) {
                assert(DataflowProgramBuilder::BindCore(b,source,expected));
                expected.core=count;second++;
            } else {
                assert(DataflowProgramBuilder::BindCore(a,count,expected));inactive++;
            }
        }
        assert(!std::memcmp(&expected,&actual,sizeof(actual)));
        count++;return true;
    }
};
int main() {
    (void)&q35_pair_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,19,1,*plan));
    auto alpha=std::make_unique<DataflowProjectionProgram>();
    auto beta=std::make_unique<DataflowProjectionProgram>();
    auto candidate=std::make_unique<DataflowProjectionAuthority>();
    auto record=std::make_unique<DataflowStreamRecord>();
    assert(candidate->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(candidate->MarkActivationInitialized(0));
    DataflowDynamicTokenContext dynamic{};
    assert(DataflowDynamicTokenContext::Build(fixture->manifest,42,1,dynamic));
    unsigned layers=0;
    for (unsigned layer=0;layer<Layers;layer++) {
        if (Dense(layer)) continue;
        assert(candidate->BuildProjection(layer,Alpha,0,DataflowGraphRecurrentAlphaOutputSlot,*alpha));
        assert(candidate->BuildProjection(layer,Beta,0,DataflowGraphRecurrentBetaOutputSlot,*beta));
        assert(alpha->activeCores==2 && beta->activeCores==2);
        assert(DataflowProjectionPair::Compatible(*alpha,*beta));
        assert(!DataflowProjectionPair::Compatible(*alpha,*alpha));
        PairSink sink{*alpha,*beta};sink.dynamic=&dynamic;
        DataflowStreamRecorder<PairSink> recorder(*candidate,*plan,sink,*record);
        assert(recorder.RunIndependentProjectionPair(*alpha,*beta,sink)==0);
        assert(!recorder.Failed() && recorder.Steps()==1 && sink.count==120
            && sink.first==2 && sink.second==2 && sink.inactive==116);
        assert(candidate->GraphWorkspaceReady(DataflowGraphRecurrentAlphaWorkspaceIndex)
            && candidate->GraphWorkspaceReady(DataflowGraphRecurrentBetaWorkspaceIndex)
            && candidate->GraphWorkspaceLayer(DataflowGraphRecurrentAlphaWorkspaceIndex)==layer
            && candidate->GraphWorkspaceLayer(DataflowGraphRecurrentBetaWorkspaceIndex)==layer);
        layers++;
    }
    assert(layers);
    assert(candidate->BuildProjection(0,Alpha,0,DataflowGraphRecurrentAlphaOutputSlot,*alpha));
    assert(candidate->BuildProjection(0,Beta,0,DataflowGraphRecurrentBetaOutputSlot,*beta));
    auto modified=std::make_unique<DataflowProjectionProgram>(*beta);
    modified->output=alpha->output;
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        assert(modified->core[core].segmentCount==alpha->core[core].segmentCount);
        for (unsigned segment=0;segment<modified->core[core].segmentCount;segment++)
            modified->core[core].output[segment]=alpha->core[core].output[segment];
    }
    assert(modified->Valid() && !DataflowProjectionPair::Compatible(*alpha,*modified));
    modified=std::make_unique<DataflowProjectionProgram>(*beta);
    modified->bankRoute[0][0]=4000;
    assert(modified->Valid() && !DataflowProjectionPair::Compatible(*alpha,*modified));
    modified=std::make_unique<DataflowProjectionProgram>(*beta);
    modified->input=fixture->manifest.activation[1];
    assert(modified->Valid() && !DataflowProjectionPair::Compatible(*alpha,*modified));
    PairSink failed{*alpha,*beta};failed.failAt=3;
    DataflowStreamRecorder<PairSink> partial(*candidate,*plan,failed,*record);
    assert(partial.RunIndependentProjectionPair(*alpha,*beta,failed)!=0
        && partial.Failed() && partial.Steps()==0 && failed.count==3);
    std::printf("PASS fused recurrent projections: %u layers, four cores, exact independent descriptors; SIMULATED ONLY\n",layers);
}
