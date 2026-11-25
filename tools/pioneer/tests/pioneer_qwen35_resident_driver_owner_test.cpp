/* SPDX-License-Identifier: MIT */
// Actual driver owner with simulated context, roles and service. Its real
// physical runtime predicate remains false; no card or device graph is used.
#include "resident-service-fixture.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ35AlignedAllocation.h"
#include "../PioneerQwen35DataflowGraphSession.h"

struct PioneerStoreContext;
struct PioneerQ35GraphOwner {};
struct TestRoleScope {
    template<class IO> bool ProveExclusiveRewrite(IO&) const { return true; }
    template<class IO> bool ProveRoleRuntimeSafe(IO&,unsigned) const { return false; }
};
struct PioneerSessionIO : ResidentRaw {
    PioneerStoreContext* qwen35DataflowContext=nullptr;
    TestRoleScope* qwen35BorrowedRoleScope=nullptr;
    unsigned runtimeCore=0;
    bool SelectRuntimeCore(unsigned core) { selected=runtimeCore=core;return core<DataflowCoreCount; }
    bool MapQ38Panel() const { return mapped; }
    bool ReadCoreMemory(unsigned,unsigned,void*,unsigned) { return false; }
};
struct PioneerStoreContext {
    PioneerSessionIO io;
    PioneerQ35GraphOwner* qwen35GraphOwner=nullptr;
    DataflowServiceOwner* qwen35DataflowService=nullptr;
    DataflowProjectionAuthority* qwen35DataflowAuthority=nullptr;
    DataflowProjectionProgram* qwen35DataflowProgramScratch=nullptr;
    void* qwen35DataflowMemory=nullptr;
    unsigned qwen35DataflowRoutes[2][Banks]{};
    bool qwen35DataflowRoutesAudited=true,q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    bool bankStorageOnly=true,initialized=true;
    size_t bytes=1;
    bool Unsafe(bool) const { return false; }
};
namespace PioneerResidentBanks { constexpr size_t Total=1; }
struct TestRuntimeOwner { unsigned magic=0x50313030;PioneerStoreContext* context=nullptr; };
static TestRuntimeOwner* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static unsigned sReloadGeneration=1;
struct PioneerQ35StreamRecordingWitness {
    const DataflowStreamPlan* plan;const void* reserved=nullptr;bool sealed=true;
    const DataflowStreamPlan* SealedPlan() const { return plan; }
    unsigned SealedSlot() const { return 0; }
    bool SealedMatches(const DataflowStreamPlan& p,unsigned slot) const { return sealed && &p==plan && !slot; }
    bool ReserveResidentSession(const void* owner,const DataflowStreamPlan& p,unsigned slot) {
        if (!owner || reserved || !SealedMatches(p,slot)) return false;
        reserved=owner;return true;
    }
    bool CancelResidentReservation(const void* owner) { if (reserved!=owner) return false;reserved=nullptr;return true; }
    bool OwnsResidentSession(const void* owner,const DataflowStreamPlan& p,unsigned slot) const {
        return reserved==owner && SealedMatches(p,slot);
    }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentDriverOwner.inc"

int main() {
    (void)&q35_service_owner_fixture_main;(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    auto service=std::make_unique<DataflowServiceOwner>();auto serviceIO=std::make_unique<MockDataflowIO>();
    uint32_t code[96]{},dispatch[80]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*serviceIO)==DataflowServiceComplete);
    auto authority=std::make_unique<DataflowProjectionAuthority>();
    assert(authority->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
    auto scratch=std::make_unique<DataflowProjectionProgram>();auto context=std::make_unique<PioneerStoreContext>();
    PioneerQ35GraphOwner graph;TestRoleScope role;TestRuntimeOwner runtime{};runtime.context=context.get();sRuntimeOwner=&runtime;
    context->io.qwen35DataflowContext=context.get();context->io.qwen35BorrowedRoleScope=&role;
    context->qwen35GraphOwner=&graph;context->qwen35DataflowService=service.get();
    context->qwen35DataflowAuthority=authority.get();context->qwen35DataflowProgramScratch=scratch.get();
    context->qwen35DataflowMemory=context.get();memcpy(context->qwen35DataflowRoutes,f->routes,sizeof(f->routes));
    PioneerQ35StreamRecordingWitness stream{plan.get()};
    context->q35PciReservationHeld=false;
    auto* rejected=PioneerQ35ResidentDriverCreate(*context,&graph,stream);
    assert(rejected && !PioneerQ35ResidentDriverBind(rejected) && service->Ready()
        && PioneerQ35ResidentDriverDestroy(rejected) && !rejected);
    context->q35PciReservationHeld=true;
    auto* owner=PioneerQ35ResidentDriverCreate(*context,&graph,stream);
    assert(owner && PioneerQ35ResidentDriverBind(owner) && PioneerQ35ResidentDriverReady(owner,0));
    assert(!service->Ready() && !PioneerQ35ResidentDriverRetained(owner));
    DataflowArgmaxResult output{};unsigned stage=0;
    assert(PioneerQ35ResidentDriverRun(owner,0,42,true,3,output,stage)==DataflowGraphSessionPreflight);
    assert(!context->io.writes && output.token==Vocabulary && !PioneerQ35ResidentDriverRetained(owner)
        && PioneerQ35ResidentDriverReady(owner,0) && !service->NextSequence());
    stream.sealed=false;
    assert(!PioneerQ35ResidentDriverReady(owner,0)
        && PioneerQ35ResidentDriverRun(owner,0,42,true,3,output,stage)==DataflowGraphSessionInvalid);
    stream.sealed=true;runtime.magic=0;
    assert(PioneerQ35ResidentDriverRun(owner,0,42,true,3,output,stage)==DataflowGraphSessionInvalid
        && !context->io.writes && !service->NextSequence());
    runtime.magic=0x50313030;
    assert(!PioneerQ35ResidentDriverDestroy(owner)); // bound service requires a qualified stop
    PioneerQ35DeleteAligned(owner);sRuntimeOwner=nullptr; // test teardown; simulated resources only
    puts("PASS actual resident driver owner binds sealed plan and denies unqualified publication; SIMULATED ONLY");
}
