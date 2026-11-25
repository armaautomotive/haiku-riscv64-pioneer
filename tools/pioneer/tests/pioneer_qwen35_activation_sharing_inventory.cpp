/* SPDX-License-Identifier: MIT */
// Inventory potential activation sharing in a synthetic, recorded model graph.
// This plans no peer transfer and provides no device or multicast authority.
#define main q35_activation_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowActivationReuse.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"
#include <cstring>
#include <vector>

struct InputGroup {
    RowSpace input{};
    unsigned base=0,rows=0,packing=0,routes[Banks]{};
    unsigned workers=0,first=DataflowCoreCount,last=0;
    uint32_t members[(DataflowCoreCount+31)/32]{};
};
struct Step {
    std::vector<InputGroup> inputs;
    std::vector<RowSpace> outputs;
};
struct Inventory {
    Step step[DataflowGraphDecodeSteps];
    unsigned active=0,inactive=0;
    uint64_t separateBytes=0;
    bool Begin(const DataflowStreamPlan&) { return true; }
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned index,
        const DataflowStreamRecord& record) {
        if (record.header[5]!=DataflowProgramMagic) return true;
        DataflowCoreInvocation invocation{};
        std::memcpy(&invocation,record.payload,sizeof(invocation));
        assert(index<DataflowGraphDecodeSteps && invocation.core==binding.core && invocation.Valid());
        if (!invocation.assignment.active) { ++inactive;return true; }
        auto& current=step[index];
        const unsigned packing=DataflowActivationReusePacking(invocation.type);
        assert(packing);
        InputGroup* group=nullptr;
        for (auto& candidate:current.inputs) {
            bool same=candidate.base==invocation.inputBase && candidate.rows==invocation.inputRows
                && candidate.packing==packing && DataflowRowSpaceMatches(candidate.input,invocation.input);
            for (unsigned bank=0;bank<Banks;bank++)
                same=same && candidate.routes[bank]==invocation.bankRoute[0][bank];
            if (same) { group=&candidate;break; }
        }
        if (!group) {
            current.inputs.emplace_back();group=&current.inputs.back();
            group->input=invocation.input;group->base=invocation.inputBase;
            group->rows=invocation.inputRows;group->packing=packing;
            for (unsigned bank=0;bank<Banks;bank++) group->routes[bank]=invocation.bankRoute[0][bank];
        }
        const uint32_t bit=uint32_t(1)<<(invocation.core%32);
        assert(!(group->members[invocation.core/32]&bit));
        group->members[invocation.core/32]|=bit;++group->workers;
        if (invocation.core<group->first) group->first=invocation.core;
        if (invocation.core>group->last) group->last=invocation.core;
        bool outputSeen=false;
        for (const auto& output:current.outputs)
            outputSeen|=DataflowRowSpaceMatches(output,invocation.output);
        if (!outputSeen) current.outputs.push_back(invocation.output);
        ++active;separateBytes+=uint64_t(invocation.inputRows)*sizeof(float);
        return true;
    }
    bool Seal(const DataflowStreamPlan&) { return true; }
    void Abort() { assert(false); }
};
int main() {
    (void)&q35_activation_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto live=std::make_unique<DataflowProjectionAuthority>();
    auto after=std::make_unique<DataflowProjectionAuthority>();
    auto plan=std::make_unique<DataflowStreamPlan>();
    auto record=std::make_unique<DataflowStreamRecord>();
    auto projection=std::make_unique<DataflowProjectionProgram>();
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
    auto inventory=std::make_unique<Inventory>();unsigned stage=0;
    assert(DataflowStreamRecording::Build(fixture->packed,*live,*after,*plan,fixture->routes,
        0,42,0,true,*inventory,*record,*projection,stage));
    assert(stage==DataflowDecoderSequenceComplete);
    uint64_t idealBytes=0;unsigned groups=0,steps=0,multiple=0,maxGroups=0,unsafe=0,noncontiguous=0;
    bool shownMixed=false;
    for (const auto& step:inventory->step) {
        if (step.inputs.empty()) continue;
        ++steps;groups+=step.inputs.size();multiple+=step.inputs.size()>1;
        if (step.inputs.size()>maxGroups) maxGroups=step.inputs.size();
        for (const auto& input:step.inputs) {
            assert(input.workers && input.first<=input.last);
            noncontiguous+=input.last-input.first+1!=input.workers;
            if (step.inputs.size()>1 && !shownMixed)
                std::printf("mixed input group: packing %u, %u workers, range %u..%u\n",
                    input.packing,input.workers,input.first,input.last);
            idealBytes+=uint64_t(input.rows)*sizeof(float);
            for (const auto& output:step.outputs)
                if (!DataflowActivationReuseDisjoint(input.input,output)) ++unsafe;
        }
        if (step.inputs.size()>1) shownMixed=true;
    }
    assert(groups && groups<=inventory->active && idealBytes<=inventory->separateBytes && !unsafe);
    std::printf("SIMULATED synthetic graph: %u projection steps, %u active shards, %u inactive, "
        "%u input/packing/route groups, %u steps with multiple groups, maximum %u, %u noncontiguous; "
        "raw input bytes %llu separate versus %llu once per group; prior-step reuse excluded\n",
        steps,inventory->active,inventory->inactive,groups,multiple,maxGroups,noncontiguous,
        (unsigned long long)inventory->separateBytes,(unsigned long long)idealBytes);
}
