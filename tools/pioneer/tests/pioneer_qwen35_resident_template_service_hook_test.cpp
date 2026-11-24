/* SPDX-License-Identifier: MIT */
// Verify the real service owner invokes resident staging and a final audit
// after all 120 workers are held and before any worker can start.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define main q35_service_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>
struct TemplateHook {
    bool stageOkay=true,verifyOkay=true;
    unsigned stages=0,verifies=0;
    bool Stage(MockDataflowIO& io) {
        ++stages;assert(io.roleStages==1 && !io.roleArms && !io.roleStarts);
        for (unsigned core=0;core<DataflowCoreCount;core++) assert(io.held[core] && !io.started[core]);
        return stageOkay;
    }
    bool Verify(MockDataflowIO& io) {
        ++verifies;assert(stages==1 && io.roleArms==1 && !io.roleStarts);
        for (unsigned core=0;core<DataflowCoreCount;core++) assert(io.held[core] && !io.started[core]);
        return verifyOkay;
    }
};
int main() {
    (void)&q35_service_fixture_main;
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    {
        auto io=std::make_unique<MockDataflowIO>();auto owner=std::make_unique<DataflowServiceOwner>();TemplateHook hook;
        assert(owner->Install(image,*io,hook)==DataflowServiceComplete && owner->Ready()
            && hook.stages==1 && hook.verifies==1 && io->roleStarts==DataflowCoreCount);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();auto owner=std::make_unique<DataflowServiceOwner>();TemplateHook hook;
        hook.stageOkay=false;
        assert(owner->Install(image,*io,hook)==DataflowServiceStartupWriteFailed && owner->Faulted()
            && hook.stages==1 && !hook.verifies && !io->roleArms && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();auto owner=std::make_unique<DataflowServiceOwner>();TemplateHook hook;
        hook.verifyOkay=false;
        assert(owner->Install(image,*io,hook)==DataflowServiceStartFailed && owner->Faulted()
            && hook.stages==1 && hook.verifies==1 && io->roleArms==1 && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    puts("PASS resident template hook stages and rechecks before any release; SIMULATED ONLY");
}
