/* SPDX-License-Identifier: MIT */
// Test the fixed diagnostic adapter with the production service owner and
// simulated queued roles. This grants no hardware execution authority.
#define PIONEER_QWEN35_PROJECTION_SERVICE_MOCK_ONLY 1
#include "pioneer_qwen35_dataflow_projection_service_integration_test.cpp"
#include "../PioneerQwen35DataflowProjectionQualificationService.h"

struct QualificationScope;
struct QualificationScratch {
    bool staged=true,running=false;
    bool IsStaged() const { return staged; }
    bool MatchesStagedLease(unsigned bank,unsigned address,unsigned bytes) const {
        return staged && bank==0 && address==BankBase && bytes==Fixture::ScratchBytes;
    }
    bool MarkRunning(QualificationScope&);
};
struct QualificationScope {
    QualificationScratch* expected=nullptr;
    ProjectionServiceIO* io=nullptr;
    bool safe=true,retained=false,failAudit=false,failAfterPublish=false;
    unsigned audits=0;
    bool QualificationLeaseMatches(const QualificationScratch& s,unsigned bank,unsigned address,unsigned bytes,
        const unsigned (&routes)[2][Banks]) const {
        return safe && &s==expected && bank==0 && address==BankBase && bytes==Fixture::ScratchBytes
            && DataflowBankRoutesValid(routes);
    }
    bool QualificationExecutionSafe(const QualificationScratch& s,const DataflowProjectionProgram& p) const {
        return safe && &s==expected && (s.staged || s.running) && p.Valid()
            && (!failAfterPublish || (io && !io->mailbox[0].commandSequence));
    }
    bool AuditQualification(ProjectionServiceIO& io,const DataflowProjectionProgram& p,unsigned seq,
        const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) {
        audits++;
        if (failAudit || !io.MockDataflowIO::AuditProjection(p,seq,receipts)) return false;
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            DataflowCoreInvocation expectedInvocation{};
            if (!DataflowProgramBuilder::BindCore(p,c,expectedInvocation)
                || std::memcmp(&expectedInvocation,&io.invocation[c],sizeof(expectedInvocation))) return false;
        }
        return true;
    }
    void RetainQualification(const void*,const void* s) { assert(s==expected);retained=true; }
};
bool QualificationScratch::MarkRunning(QualificationScope& scope) {
    if (!staged || running || !scope.safe || scope.expected!=this) return false;
    staged=false;running=true;return true;
}

static void InitializeFixture(ProjectionServiceIO& io) {
    assert(Fixture::FillWeightBytes(0,io.memory.weights.data(),Fixture::WeightBytes));
    assert(Fixture::FillInputBytes(0,io.memory.input.data(),Fixture::InputBytes));
    assert(Fixture::FillOutputBytes(0,io.memory.output.data(),Fixture::OutputBytes));
}

int main()
{
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    for (unsigned failure=0;failure<4;failure++) {
        auto io=std::make_unique<ProjectionServiceIO>();
        auto service=std::make_unique<DataflowServiceOwner>();
        auto qualification=std::make_unique<DataflowProjectionQualificationService>();
        QualificationScratch scratch;QualificationScope scope;scope.expected=&scratch;scope.io=io.get();
        InitializeFixture(*io);
        assert(service->Install(image,*io)==DataflowServiceComplete);
        assert(!qualification->Bind(scratch,scope,0,BankBase+4096,TestRoutes));
        assert(qualification->Bind(scratch,scope,0,BankBase,TestRoutes));
        auto copy=std::make_unique<DataflowProjectionProgram>(*qualification->Program());
        assert(!qualification->Admits(*copy,scratch,scope));
        assert(qualification->Admits(*qualification->Program(),scratch,scope));
        scope.safe=false;
        assert(qualification->Run(*service,*io,scratch,scope)==DataflowServiceInvalidState
            && service->Ready() && scratch.IsStaged());
        scope.safe=true;
        if (failure==1) io->failWorkerCore=37;
        if (failure==2) scope.failAudit=true;
        if (failure==3) scope.failAfterPublish=true;
        const auto result=qualification->Run(*service,*io,scratch,scope);
        if (failure) {
            assert(result==DataflowServiceProjectionFailed && qualification->IsFailed()
                && service->Faulted() && scratch.running && scope.retained
                && scope.audits==(failure==2?1U:0U));
            if (failure==3) assert(io->mailbox[0].commandSequence==1 && !io->mailbox[1].commandSequence);
        } else {
            assert(result==DataflowServiceComplete && qualification->IsAudited()
                && service->Ready() && scratch.running && scope.audits==1 && !scope.retained);
            for (unsigned row=0;row<Fixture::Rows;row++) {
                uint32_t bits=0;std::memcpy(&bits,io->memory.output.data()+row*4,4);
                assert(Fixture::OutputMatches(row,bits));
            }
        }
        assert(!qualification->Admits(*qualification->Program(),scratch,scope));
        assert(service->Close(*io)==DataflowServiceClosed);
    }
    std::puts("PASS one-shot fixed projection authorization, audited service and fault retention; SIMULATED ONLY");
    return 0;
}
