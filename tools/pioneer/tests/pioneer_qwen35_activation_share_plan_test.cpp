/* SPDX-License-Identifier: MIT */
// Exercise the actual share planner with actual synthetic graph records.
#define main q35_share_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowActivationSharePlan.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"
#include <cstring>

static bool SameInput(const DataflowCoreInvocation& a,const DataflowCoreInvocation& b) {
    if (!a.assignment.active || !b.assignment.active || a.inputBase!=b.inputBase
        || a.inputRows!=b.inputRows || DataflowActivationReusePacking(a.type)!=DataflowActivationReusePacking(b.type)
        || !DataflowRowSpaceMatches(a.input,b.input)) return false;
    for (unsigned bank=0;bank<Banks;bank++) if (a.bankRoute[0][bank]!=b.bankRoute[0][bank]) return false;
    return true;
}
struct PlanSink {
    std::unique_ptr<DataflowActivationSharePlan> plan;
    std::unique_ptr<DataflowCoreInvocation[]> commands;
    unsigned index=UINT32_MAX,seen=0,steps=0,active=0,multiple=0;
    bool negativesChecked=false,overlapChecked=false;
    bool Finish() {
        if (!plan) return true;
        assert(seen==DataflowCoreCount && plan->Seal() && plan->Groups());
        ++steps;multiple+=plan->Groups()>1;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            const auto& command=commands[core];DataflowActivationShareMembership group{};
            if (!command.assignment.active) { assert(!plan->Get(core,group));continue; }
            ++active;assert(plan->Get(core,group) && group.packing==DataflowActivationReusePacking(command.type)
                && group.inputKey==DataflowActivationShareInputKey(command));
            unsigned leader=DataflowCoreCount;
            for (unsigned peer=0;peer<DataflowCoreCount;peer++) {
                const bool member=(group.members[peer/32]>>(peer%32))&1U;
                assert(member==SameInput(command,commands[peer]));
                if (member && peer<leader) leader=peer;
            }
            assert(group.leader==leader && !(group.members[3]&0xff000000U));
        }
        if (!negativesChecked) {
            auto reverse=std::make_unique<DataflowActivationSharePlan>();
            for (unsigned core=DataflowCoreCount;core>0;core--) assert(reverse->Add(commands[core-1]));
            assert(reverse->Seal() && reverse->Groups()==plan->Groups());
            for (unsigned core=0;core<DataflowCoreCount;core++) {
                DataflowActivationShareMembership first{},second{};
                const bool original=plan->Get(core,first),reordered=reverse->Get(core,second);
                assert(original==reordered);
                if (original) {
                    assert(first.leader==second.leader && first.packing==second.packing
                        && first.inputKey==second.inputKey);
                    for (unsigned word=0;word<DataflowActivationShareMaskWords;word++)
                        assert(first.members[word]==second.members[word]);
                }
            }
            auto duplicate=std::make_unique<DataflowActivationSharePlan>();
            assert(duplicate->Add(commands[0]) && !duplicate->Add(commands[0]) && !duplicate->Seal());
            auto incomplete=std::make_unique<DataflowActivationSharePlan>();
            for (unsigned core=0;core+1<DataflowCoreCount;core++) assert(incomplete->Add(commands[core]));
            assert(!incomplete->Seal());
            auto invalid=std::make_unique<DataflowActivationSharePlan>();
            auto bad=commands[0];bad.core=DataflowCoreCount;
            assert(!invalid->Add(bad) && !invalid->Seal());
            negativesChecked=true;
        }
        if (!overlapChecked) for (unsigned core=0;core<DataflowCoreCount;core++) {
            auto bad=commands[core];
            if (!bad.assignment.active || bad.inputBase>bad.output.rows
                || bad.inputRows>bad.output.rows-bad.inputBase) continue;
            bad.input=bad.output;bad.inputFragmentCount=bad.input.count;
            if (!bad.Valid()) continue;
            assert(!DataflowActivationReuseDisjoint(bad.input,bad.output));
            auto overlap=std::make_unique<DataflowActivationSharePlan>();
            for (unsigned i=0;i<DataflowCoreCount;i++) assert(overlap->Add(i==core?bad:commands[i]));
            DataflowActivationShareMembership rejected{};
            assert(!overlap->Seal() && !overlap->Get(core,rejected));
            overlapChecked=true;break;
        }
        plan.reset();commands.reset();seen=0;return true;
    }
    bool Begin(const DataflowStreamPlan&) { return true; }
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,
        const DataflowStreamRecord& record) {
        if (step!=index) { assert(Finish());index=step; }
        if (record.header[5]!=DataflowProgramMagic) return true;
        if (!plan) {
            plan=std::make_unique<DataflowActivationSharePlan>();
            commands=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
        }
        assert(binding.core<DataflowCoreCount && binding.core==seen);
        std::memcpy(&commands[binding.core],record.payload,sizeof(DataflowCoreInvocation));
        assert(commands[binding.core].core==binding.core && plan->Add(commands[binding.core]));
        ++seen;return true;
    }
    bool Seal(const DataflowStreamPlan&) { return Finish(); }
    void Abort() { assert(false); }
};
int main() {
    (void)&q35_share_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto live=std::make_unique<DataflowProjectionAuthority>();
    auto after=std::make_unique<DataflowProjectionAuthority>();
    auto stream=std::make_unique<DataflowStreamPlan>();
    auto record=std::make_unique<DataflowStreamRecord>();
    auto projection=std::make_unique<DataflowProjectionProgram>();
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*stream));
    PlanSink sink;unsigned stage=0;
    assert(DataflowStreamRecording::Build(fixture->packed,*live,*after,*stream,fixture->routes,
        0,42,0,true,sink,*record,*projection,stage));
    assert(stage==DataflowDecoderSequenceComplete && sink.negativesChecked && sink.overlapChecked
        && sink.steps && sink.active);
    std::printf("PASS %u recorded projection steps, %u active shards, %u mixed-group steps; "
        "exact peer masks and retained planner failures; SIMULATED ONLY\n",sink.steps,sink.active,sink.multiple);
}
