/* SPDX-License-Identifier: MIT */
#define main q35_triple_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include <vector>

struct TripleSink {
    unsigned count=0;int failAt=-1;
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,const DataflowStreamRecord& record) {
        assert(step==0 && binding.core==count);
        if (int(count)==failAt) return false;
        assert(DataflowStreamRecordStructureValid(record,binding.epoch,1,binding.core));++count;return true;
    }
};
int main() {
    (void)&q35_triple_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto authority=std::make_unique<DataflowProjectionAuthority>();
    auto full=std::make_unique<DataflowProjectionProgram>();
    auto qkv=std::make_unique<DataflowProjectionProgram>();
    auto alpha=std::make_unique<DataflowProjectionProgram>();
    auto beta=std::make_unique<DataflowProjectionProgram>();
    assert(authority->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(authority->MarkActivationInitialized(0));
    unsigned layers=0,lastLayer=0;
    for (unsigned layer=0;layer<Layers;layer++) {
        if (Dense(layer)) continue;
        assert(authority->BuildProjection(layer,RecurrentQkv,0,DataflowGraphRecurrentQkvOutputSlot,*full));
        assert(authority->BuildProjection(layer,Alpha,0,DataflowGraphRecurrentAlphaOutputSlot,*alpha));
        assert(authority->BuildProjection(layer,Beta,0,DataflowGraphRecurrentBetaOutputSlot,*beta));
        assert(full->activeCores==120 && alpha->activeCores==2 && beta->activeCores==2);
        assert(!DataflowProjectionTriple::Compatible(*full,*alpha,*beta));
        assert(authority->BuildProjection(layer,RecurrentQkv,0,DataflowGraphRecurrentQkvOutputSlot,*qkv,116)
            && qkv->activeCores==116 && authority->Authorize(*qkv));
        assert(DataflowProjectionTriple::Compatible(*qkv,*alpha,*beta));
        std::vector<unsigned> qkvHits(qkv->groups),alphaHits(alpha->groups),betaHits(beta->groups);
        unsigned qkvCores=0,alphaCores=0,betaCores=0;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowCoreInvocation invocation{};
            assert(DataflowProjectionTriple::BindCore(*qkv,*alpha,*beta,core,invocation)
                && invocation.Valid() && invocation.assignment.active && invocation.core==core);
            std::vector<unsigned>* hits=nullptr;
            if (DataflowRowSpaceMatches(invocation.output,qkv->output)) { hits=&qkvHits;++qkvCores; }
            else if (DataflowRowSpaceMatches(invocation.output,alpha->output)) { hits=&alphaHits;++alphaCores; }
            else if (DataflowRowSpaceMatches(invocation.output,beta->output)) { hits=&betaHits;++betaCores; }
            assert(hits);
            for (unsigned i=0;i<invocation.assignment.segmentCount;i++) {
                const auto& segment=invocation.assignment.weight[i];
                for (unsigned g=0;g<segment.groups;g++) {
                    assert(segment.firstGroup+g<hits->size());++(*hits)[segment.firstGroup+g];
                }
            }
            DataflowStreamRecord record{};
            assert(DataflowStreamEncode(invocation,19,1,record)
                && DataflowStreamRecordStructureValid(record,19,1,core));
        }
        assert(qkvCores==116 && alphaCores==2 && betaCores==2);
        for (unsigned hit:qkvHits) assert(hit==1);
        for (unsigned hit:alphaHits) assert(hit==1);
        for (unsigned hit:betaHits) assert(hit==1);
        assert(!DataflowProjectionTriple::Compatible(*qkv,*alpha,*alpha));
        lastLayer=layer;++layers;
    }
    assert(layers==24);
    auto invalid=std::make_unique<DataflowProjectionProgram>();
    assert(!authority->BuildProjection(lastLayer,RecurrentQkv,0,DataflowGraphRecurrentQkvOutputSlot,*invalid,0));
    assert(!authority->BuildProjection(lastLayer,RecurrentQkv,0,DataflowGraphRecurrentQkvOutputSlot,*invalid,121));
    auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,19,1,*plan));
    for (int fail:{-1,0,116,118,119}) {
        auto candidate=std::make_unique<DataflowProjectionAuthority>();
        assert(candidate->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope)
            && candidate->MarkActivationInitialized(0));
        TripleSink sink;sink.failAt=fail;
        DataflowStreamRecorder<TripleSink> recorder(*candidate,*plan,sink,*record);
        const int result=recorder.RunIndependentProjectionTriple(*qkv,*alpha,*beta,sink);
        if (fail>=0) assert(result!=0 && recorder.Failed() && recorder.Steps()==0 && sink.count==unsigned(fail));
        else assert(!result && !recorder.Failed() && recorder.Steps()==1 && sink.count==120
            && candidate->GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
            && candidate->GraphWorkspaceReady(DataflowGraphRecurrentAlphaWorkspaceIndex)
            && candidate->GraphWorkspaceReady(DataflowGraphRecurrentBetaWorkspaceIndex)
            && candidate->GraphWorkspaceLayer(DataflowGraphRecurrentQkvWorkspaceIndex)==lastLayer);
    }
    std::printf("PASS three recurrent projections: %u layers, 116+2+2 cores, exact shards; SIMULATED ONLY\n",layers);
}
