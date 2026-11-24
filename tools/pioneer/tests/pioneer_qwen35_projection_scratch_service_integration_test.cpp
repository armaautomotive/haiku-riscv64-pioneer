/* SPDX-License-Identifier: MIT */
// Actual scratch transaction, diagnostic authorization and service owner,
// connected over a simulated NoC and test-only matrix model. No hardware.
#define PIONEER_QWEN35_PROJECTION_SERVICE_MOCK_ONLY 1
#include "pioneer_qwen35_dataflow_projection_service_integration_test.cpp"
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../PioneerQwen35DataflowProjectionQualificationService.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"

struct CombinedScope : ScratchScope {
    ScratchDevice<>& device;
    DataflowServiceOwner* service=nullptr;
    PioneerQ35ProjectionQualificationOwner* holder=nullptr;
    bool executionAllowed=true;
    explicit CombinedScope(ScratchDevice<>& d):device(d) {}
    bool QualificationLeaseMatches(const PioneerQ35ProjectionScratch& s,unsigned bank,unsigned address,
        unsigned bytes,const unsigned (&routes)[2][Banks]) const {
        if (!allowed || owner!=&s || !s.MatchesStagedLease(bank,address,bytes)
            || !s.MatchesBankClaim(device.banks,device.dram)
            || !DataflowBankRoutesValid(routes)) return false;
        for (unsigned b=0;b<Banks;b++) for (unsigned noc=0;noc<2;noc++)
            if (routes[noc][b]!=(device.dram.translated[b][0]|device.dram.translated[b][1]<<6)) return false;
        return true;
    }
    bool QualificationExecutionSafe(const PioneerQ35ProjectionScratch& s,
        const DataflowProjectionProgram& p) const {
        return allowed && executionAllowed && owner==&s && (s.IsStaged() || s.IsRunning()) && p.Valid()
            && s.MatchesBankClaimOwner(device.banks,device.dram);
    }
    bool AuditQualification(ProjectionServiceIO& io,const DataflowProjectionProgram& p,unsigned seq,
        const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) {
        if (!allowed || !io.MockDataflowIO::AuditProjection(p,seq,receipts)) return false;
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            DataflowCoreInvocation invocation{};
            if (!DataflowProgramBuilder::BindCore(p,c,invocation)
                || std::memcmp(&invocation,&io.invocation[c],sizeof(invocation))) return false;
        }
        return true;
    }
    void RetainScratch(const void* s) {
        ScratchScope::RetainScratch(s);if (holder) holder->Retain();
    }
    void RetainQualification(const void*,const void* s) {
        assert(s==owner);retained=true;if (holder) holder->Retain();
    }
    template<class Context>
    bool QualificationRunAllowed(const PioneerQ35ProjectionQualificationOwner& h,
        const PioneerQ35ProjectionScratch& s,const DataflowProjectionQualificationService&,const Context& c) const {
        return allowed && c.qwen35QualificationInternalActive && h.Owns(s);
    }
    bool QualificationOwnerRetirementAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& s,const DataflowProjectionQualificationService& q) const {
        return allowed && stopped && !retained && service && service->Closed()
            && holder.Owns(s) && s.IsRestored() && q.IsAudited();
    }
    bool QualificationOwnerAbandonAllowed(const PioneerQ35ProjectionQualificationOwner&) const {
        return allowed && !retained;
    }
    bool QualificationOwnerAbandonRestoredAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& s,const DataflowProjectionQualificationService& q) const {
        return allowed && !retained && holder.Owns(s) && s.IsRestored() && q.IsUnstarted()
            && (!service || service->Closed());
    }
};

struct QualificationContext {
    PioneerQ35ProjectionQualificationOwner& qwen35QualificationOwner;
    bool qwen35QualificationInternalActive=false;
    DataflowServiceOwner* qwen35DataflowService;
    ProjectionServiceIO& io;
    QualificationContext(PioneerQ35ProjectionQualificationOwner& h,DataflowServiceOwner& s,ProjectionServiceIO& i)
        :qwen35QualificationOwner(h),qwen35DataflowService(&s),io(i) {}
#include "pioneer_q35_projection_internal_scope.inc"
#include "pioneer_q35_projection_run.inc"
};

int main()
{
    (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
    auto holder=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
    assert(holder->Allocate() && holder->Active() && !holder->Allocate());
    auto* scratch=holder->Scratch();auto* qualification=holder->Qualification();
    assert(scratch && qualification && holder->Snapshot());
    auto device=std::make_unique<ScratchDevice<>>();
    auto io=std::make_unique<ProjectionServiceIO>();
    io->memory.base=Fixture::ScratchAddress;
    auto service=std::make_unique<DataflowServiceOwner>();
    CombinedScope scope(*device);
    scope.service=service.get();
    scope.holder=holder.get();
    assert(scratch->Capture(device->io,scope,device->dram,device->noc,device->host,device->lease,
        device->banks,0,Fixture::ScratchOffset,device->initialized.data(),device->initialized.size(),
        holder->Snapshot(),PioneerQ35ProjectionScratch::SavedWords));
    assert(!holder->Retire(scope,device->banks,device->dram));
    assert(!holder->AbandonUnclaimed(scope,device->banks));
    assert(scratch->Stage(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks));
    assert(qualification->Bind(*scratch,scope,0,Fixture::ScratchAddress,TestRoutes));
    // Copy the staged bank into the test-only device model; actual hardware
    // performs these reads through NoC, never through a CPU fallback.
    for (unsigned n=0;n<Fixture::WeightPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096));
        std::memcpy(io->memory.weights.data()+n*4096,p.data(),4096);
    }
    for (unsigned n=0;n<Fixture::InputPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),Fixture::ScratchAddress+(Fixture::WeightPages+n)*4096));
        const unsigned offset=n*4096,remaining=Fixture::InputBytes-offset;
        std::memcpy(io->memory.input.data()+offset,p.data(),remaining<4096?remaining:4096);
    }
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*io)==DataflowServiceComplete);
    QualificationContext context(*holder,*service,*io);
    scope.executionAllowed=false;
    assert(context.RunDataflowQualification(scope)==DataflowServiceInvalidState
        && holder->Active() && !holder->Retained() && scratch->IsStaged() && service->Ready()
        && !context.qwen35QualificationInternalActive);
    scope.executionAllowed=true;
    assert(context.RunDataflowQualification(scope)==DataflowServiceComplete
        && qualification->IsAudited() && scratch->IsRunning());
    assert(!scratch->Restore(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks));
    // Model the device writer placing its results in the claimed bank.
    for (unsigned n=0;n<Fixture::OutputPages;n++) {
        auto& p=device->io.memory[device->io.Key(18|(12<<6),Fixture::ScratchAddress
            +(Fixture::WeightPages+Fixture::InputPages+n)*4096)];
        const unsigned offset=n*4096,remaining=Fixture::OutputBytes-offset;
        std::memcpy(p.data(),io->memory.output.data()+offset,remaining<4096?remaining:4096);
    }
    assert(service->Close(*io)==DataflowServiceClosed);
    scope.stopped=true;
    assert(scratch->MarkStopped(scope));
    assert(scratch->VerifyOutput(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks));
    assert(scratch->Restore(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks)
        && scratch->IsRestored() && !scope.retained);
    assert(!holder->AbandonRestoredUnrun(scope,device->banks,device->dram)); // this run was published
    assert(holder->Retire(scope,device->banks,device->dram) && holder->Retired() && !holder->Active()
        && device->banks.Q35StreamClaimed());
    for (unsigned n=0;n<Fixture::ScratchPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096));
        for (unsigned i=0;i<1024;i++) assert(p[i]==device->original[n*1024+i]);
    }
    PioneerResidentBanks unclaimed;
    PioneerQ35ProjectionQualificationOwner unused;
    assert(unused.Allocate() && unused.AbandonUnclaimed(scope,unclaimed) && unused.Retired());
    PioneerQ35ProjectionQualificationOwner retained;
    assert(retained.Allocate());retained.Retain();
    assert(retained.Active() && retained.Retained() && !retained.AbandonUnclaimed(scope,unclaimed));
    {
        auto failedHolder=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
        assert(failedHolder->Allocate());
        auto failedDevice=std::make_unique<ScratchDevice<>>();
        CombinedScope failedScope(*failedDevice);failedScope.holder=failedHolder.get();
        auto* failedScratch=failedHolder->Scratch();
        assert(failedScratch->Capture(failedDevice->io,failedScope,failedDevice->dram,failedDevice->noc,
            failedDevice->host,failedDevice->lease,failedDevice->banks,0,Fixture::ScratchOffset,
            failedDevice->initialized.data(),failedDevice->initialized.size(),failedHolder->Snapshot(),
            PioneerQ35ProjectionScratch::SavedWords));
        failedDevice->io.failRequest=failedDevice->io.doorbells+1;
        assert(!failedScratch->Stage(failedDevice->io,failedScope,failedDevice->dram,failedDevice->noc,
            failedDevice->host,failedDevice->lease,failedDevice->banks));
        assert(failedHolder->Active() && failedHolder->Retained() && failedScope.retained
            && !failedHolder->AbandonUnclaimed(failedScope,failedDevice->banks)
            && failedDevice->banks.Q35StreamClaimed());
    }
    std::puts("PASS claimed scratch, 120-worker resident projection, result and full restoration; SIMULATED ONLY");
    return 0;
}
