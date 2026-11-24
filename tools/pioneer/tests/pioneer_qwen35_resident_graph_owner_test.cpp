/* SPDX-License-Identifier: MIT */
// The runner extracts the real driver method into resident-owner-method.inc.
// Its service, template installer and graph plans are real; IO is simulated.
#define Q35_RESIDENT_FIXTURE_ONLY
#include "pioneer_qwen35_resident_template_service_integration_test.cpp"

struct TestContext;
struct TestSessionIO : ResidentServiceIO {
    TestContext* qwen35DataflowContext=nullptr;
    bool SharedFleetHeldAndQuiescent() { return ResidentTemplateFleetHeld(); }
};
using PioneerSessionIO=TestSessionIO;
struct TestAuthority {
    const DeviceLayout* manifest;
    const DeviceLayout* Manifest() const { return manifest; }
};
class PioneerQ35GraphOwner;
struct TestContext {
    TestSessionIO io;
    PioneerQ35GraphOwner* qwen35GraphOwner=nullptr;
    DataflowServiceOwner* qwen35DataflowService=nullptr;
    TestAuthority* qwen35DataflowAuthority=nullptr;
    void* qwen35DataflowMemory=nullptr;
    unsigned qwen35DataflowRoutes[2][Banks]{};
    bool bankStorageOnly=true,initialized=true;
    size_t bytes=1;
};
using PioneerStoreContext=TestContext;
namespace PioneerResidentBanks { constexpr size_t Total=1; }
struct TestRuntimeOwner { unsigned magic=0x50313030; TestContext* context=nullptr; };
static TestRuntimeOwner* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static unsigned sReloadGeneration=1;
struct TestStream {
    const DataflowStreamPlan* plan;
    bool sealed=true;
    const DataflowStreamPlan* SealedPlan() const { return plan; }
    unsigned SealedSlot() const { return 0; }
    bool SealedMatches(const DataflowStreamPlan& requested,unsigned slot) const {
        return sealed && &requested==plan && slot==0;
    }
};
class PioneerQ35GraphOwner {
public:
    PioneerStoreContext& context;
    TestStream stream;
    bool recorded=true,bound=false;
    PioneerQ35GraphOwner(PioneerStoreContext& c,const DataflowStreamPlan& p):context(c),stream{&p} {}
#include "resident-owner-method.inc"
};

int main() {
    (void)&q35_service_fixture_main;(void)&q35_publisher_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    for (unsigned scenario=0;scenario<6;scenario++) {
        auto context=std::make_unique<TestContext>();
        auto service=std::make_unique<DataflowServiceOwner>();
        TestAuthority authority{&fixture->manifest};
        PioneerQ35GraphOwner owner(*context,*plan);
        TestRuntimeOwner runtime{};runtime.context=context.get();sRuntimeOwner=&runtime;
        context->io.qwen35DataflowContext=context.get();context->io.plan=plan.get();
        context->qwen35GraphOwner=&owner;context->qwen35DataflowService=service.get();
        context->qwen35DataflowAuthority=&authority;context->qwen35DataflowMemory=context.get();
        memcpy(context->qwen35DataflowRoutes,fixture->routes,sizeof(fixture->routes));
        if (scenario==1) context->io.dropCore=37;
        if (scenario==2) context->io.corruptAfterArm=true;
        if (scenario==3) owner.stream.sealed=false;
        if (scenario==4) runtime.magic=0;
        if (scenario==5) context->io.qwen35DataflowContext=nullptr;
        const auto result=owner.InstallResident(image);
        if (!scenario) {
            assert(result==DataflowServiceComplete && service->Ready()
                && context->io.roleStarts==DataflowCoreCount);
        } else if (scenario==3) {
            assert(result==DataflowServiceInvalidState && !service->Started() && !context->io.roleStarts);
        } else {
            assert(result==(scenario==2?DataflowServiceStartFailed:DataflowServiceStartupWriteFailed)
                && service->Faulted() && !context->io.roleStarts);
        }
        if (service->Started()) assert(service->Close(context->io)==DataflowServiceClosed);
    }
    sRuntimeOwner=nullptr;
    puts("PASS actual resident graph-owner install, sealed owner, held fleet and retained failures; SIMULATED ONLY");
}
