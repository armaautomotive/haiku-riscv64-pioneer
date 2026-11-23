/* SPDX-License-Identifier: MIT */
// Actual scratch transaction, diagnostic authorization and service owner,
// connected over a simulated NoC and test-only matrix model. No hardware.
#define PIONEER_QWEN35_PROJECTION_SERVICE_MOCK_ONLY 1
#include "pioneer_qwen35_dataflow_projection_service_integration_test.cpp"
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../PioneerQwen35DataflowProjectionQualificationService.h"

struct CombinedScope : ScratchScope {
    ScratchDevice<>& device;
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
        return allowed && owner==&s && (s.IsStaged() || s.IsRunning()) && p.Valid()
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
    void RetainQualification(const void*,const void* s) { assert(s==owner);retained=true; }
};

int main()
{
    (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
    auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
    auto device=std::make_unique<ScratchDevice<>>();
    auto io=std::make_unique<ProjectionServiceIO>();
    auto service=std::make_unique<DataflowServiceOwner>();
    auto qualification=std::make_unique<DataflowProjectionQualificationService>();
    CombinedScope scope(*device);
    assert(scratch->Capture(device->io,scope,device->dram,device->noc,device->host,device->lease,
        device->banks,0,0,device->initialized.data(),device->initialized.size(),
        device->snapshot.data(),device->snapshot.size()));
    assert(scratch->Stage(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks));
    assert(qualification->Bind(*scratch,scope,0,BankBase,TestRoutes));
    // Copy the staged bank into the test-only device model; actual hardware
    // performs these reads through NoC, never through a CPU fallback.
    for (unsigned n=0;n<Fixture::WeightPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),PioneerResidentBanks::Base+n*4096));
        std::memcpy(io->memory.weights.data()+n*4096,p.data(),4096);
    }
    for (unsigned n=0;n<Fixture::InputPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),PioneerResidentBanks::Base+(Fixture::WeightPages+n)*4096));
        const unsigned offset=n*4096,remaining=Fixture::InputBytes-offset;
        std::memcpy(io->memory.input.data()+offset,p.data(),remaining<4096?remaining:4096);
    }
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*io)==DataflowServiceComplete);
    assert(qualification->Run(*service,*io,*scratch,scope)==DataflowServiceComplete
        && qualification->IsAudited() && scratch->IsRunning());
    assert(!scratch->Restore(device->io,scope,device->dram,device->noc,device->host,device->lease,device->banks));
    // Model the device writer placing its results in the claimed bank.
    for (unsigned n=0;n<Fixture::OutputPages;n++) {
        auto& p=device->io.memory[device->io.Key(18|(12<<6),PioneerResidentBanks::Base
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
    for (unsigned n=0;n<Fixture::ScratchPages;n++) {
        const auto& p=device->io.memory.at(device->io.Key(18|(12<<6),PioneerResidentBanks::Base+n*4096));
        for (unsigned i=0;i<1024;i++) assert(p[i]==device->original[n*1024+i]);
    }
    std::puts("PASS claimed scratch, 120-worker resident projection, result and full restoration; SIMULATED ONLY");
    return 0;
}
