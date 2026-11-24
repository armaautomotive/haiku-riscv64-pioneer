/* SPDX-License-Identifier: MIT */
// Compose the real held-template installer with the real service owner over
// simulated per-core L1. No device or inference is used.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define main q35_service_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentTemplateSetup.h"
#include <memory>
struct ResidentServiceIO : MockDataflowIO {
    uint32_t templates[DataflowCoreCount][DataflowGraphRunBytes/4]{};
    const DataflowStreamPlan* plan=nullptr;
    bool retainedTemplates=false,corruptAfterArm=false;
    int dropCore=-1;
    bool ResidentTemplateOwnerAllowed(const DataflowStreamPlan& p,const DataflowDynamicTokenContext&,unsigned slot) {
        return plan==&p && slot==0 && roleStaged && !roleStarts;
    }
    bool ResidentTemplateFleetHeld() {
        for (unsigned c=0;c<DataflowCoreCount;c++) if (!held[c]) return false;
        return true;
    }
    bool ResidentTemplateCoreHeld(unsigned c) { return CoreHeld(c); }
    void RetainResidentTemplates() { retainedTemplates=true; }
    bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes) {
        if (address==DataflowGraphRunAddress && bytes==DataflowGraphRunBytes) {
            if (!CoreHeld(core) || !input) return false;
            if (int(core)!=dropCore) memcpy(templates[core],input,bytes);
            return int(core)!=dropCore;
        }
        return MockDataflowIO::WriteCoreMemory(core,address,input,bytes);
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        if (address==DataflowGraphRunAddress && bytes==DataflowGraphRunBytes) {
            if (!CoreHeld(core) || !output) return false;
            memcpy(output,templates[core],bytes);return true;
        }
        return MockDataflowIO::ReadCoreMemory(core,address,output,bytes);
    }
    bool SharedArmAndBeginRelease() {
        if (!MockDataflowIO::SharedArmAndBeginRelease()) return false;
        if (corruptAfterArm) templates[37][0]^=1;
        return true;
    }
};
struct ResidentScope {
    ResidentServiceIO& owner;
    bool ResidentTemplateOwnerAllowed(ResidentServiceIO& io,const DataflowStreamPlan& p,
        const DataflowDynamicTokenContext& seed,unsigned slot) {
        return &io==&owner && io.ResidentTemplateOwnerAllowed(p,seed,slot);
    }
    bool ResidentTemplateFleetHeld(ResidentServiceIO& io) { return &io==&owner && io.ResidentTemplateFleetHeld(); }
    bool ResidentTemplateCoreHeld(ResidentServiceIO& io,unsigned c) { return &io==&owner && io.ResidentTemplateCoreHeld(c); }
    bool ResidentTemplateAccessAllowed(ResidentServiceIO& io,unsigned c,bool) { return &io==&owner && io.CoreHeld(c); }
    void RetainResidentTemplates() { owner.RetainResidentTemplates(); }
};
int main() {
    (void)&q35_service_fixture_main;(void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    DataflowDynamicTokenContext seed{};assert(DataflowDynamicTokenContext::Build(f->manifest,0,0,seed));
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    for (unsigned scenario=0;scenario<3;scenario++) {
        auto io=std::make_unique<ResidentServiceIO>();io->plan=plan.get();
        if (scenario==1) io->dropCore=37;
        if (scenario==2) io->corruptAfterArm=true;
        auto owner=std::make_unique<DataflowServiceOwner>();ResidentScope scope{*io};
        DataflowResidentTemplateSetup<ResidentScope> hook(*plan,f->routes,seed,0,100,scope);
        const auto result=owner->Install(image,*io,hook);
        if (!scenario) {
            assert(result==DataflowServiceComplete && owner->Ready() && io->roleStarts==DataflowCoreCount);
            for (unsigned c=0;c<DataflowCoreCount;c++) {
                DataflowGraphRunDescriptor d{};memcpy(&d,io->templates[c],sizeof(d));
                assert(DataflowGraphRunValid(d,c));
            }
        } else {
            assert(result==(scenario==1?DataflowServiceStartupWriteFailed:DataflowServiceStartFailed)
                && owner->Faulted() && !io->roleStarts && hook.Retained() && io->retainedTemplates);
        }
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    puts("PASS resident templates installed and audited inside held service lifecycle; SIMULATED ONLY");
}
