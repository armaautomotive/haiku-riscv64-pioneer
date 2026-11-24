/* SPDX-License-Identifier: MIT */
// Production projection adapter with simulated per-core route mutation.
#define PIONEER_QWEN35_PROJECTION_SERVICE_MOCK_ONLY 1
#include "pioneer_qwen35_dataflow_projection_service_integration_test.cpp"
#include "../PioneerQwen35DataflowProjectionQualificationService.h"
struct RoutedScope;
struct RoutedScratch {
    bool staged=true,running=false;
    bool IsStaged() const { return staged; }
    bool MatchesStagedLease(unsigned bank,unsigned address,unsigned bytes) const {
        return staged && bank==0 && address==BankBase && bytes==Fixture::ScratchBytes;
    }
    bool MarkRunning(RoutedScope&);
};
struct RoutedIO : ProjectionServiceIO {
    unsigned route=0,failRead=UINT32_MAX;
    bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& out) {
        route=core;
        return core!=failRead && ProjectionServiceIO::ReadWorkerSnapshot(core,out);
    }
};
struct RoutedScope {
    RoutedIO* io=nullptr;RoutedScratch* owner=nullptr;
    bool retained=false;unsigned failRestore=UINT32_MAX,restores=0;
    bool QualificationLeaseMatches(const RoutedScratch& s,unsigned bank,unsigned address,unsigned bytes,
        const unsigned (&routes)[2][Banks]) const {
        return !retained && &s==owner && bank==0 && address==BankBase && bytes==Fixture::ScratchBytes
            && DataflowBankRoutesValid(routes);
    }
    bool QualificationExecutionSafe(const RoutedScratch& s,const DataflowProjectionProgram& p) const {
        return !retained && io && io->route==0 && &s==owner && (s.staged || s.running) && p.Valid();
    }
    bool QualificationRestoreRuntimeRoute(const RoutedScratch& s,const DataflowProjectionProgram& p,RoutedIO& actual) {
        ++restores;
        if (retained || &actual!=io || &s!=owner || !p.Valid() || actual.route==failRestore) return false;
        actual.route=0;return true;
    }
    bool AuditQualification(RoutedIO& actual,const DataflowProjectionProgram& p,unsigned sequence,
        const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) {
        if (&actual!=io || !actual.MockDataflowIO::AuditProjection(p,sequence,receipts)) return false;
        actual.route=119;return true;
    }
    void RetainQualification(const void*,const void* scratch) { assert(scratch==owner);retained=true; }
};
bool RoutedScratch::MarkRunning(RoutedScope& scope) {
    if (!staged || running || scope.retained || scope.owner!=this) return false;
    staged=false;running=true;return true;
}
int main() {
    uint32_t code[96]{},dispatch[80]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    for (unsigned scenario=0;scenario<3;scenario++) {
        auto io=std::make_unique<RoutedIO>();auto service=std::make_unique<DataflowServiceOwner>();
        auto qualification=std::make_unique<DataflowProjectionQualificationService>();
        RoutedScratch scratch;RoutedScope scope;scope.io=io.get();scope.owner=&scratch;
        assert(Fixture::FillWeightBytes(0,io->memory.weights.data(),Fixture::WeightBytes));
        assert(Fixture::FillInputBytes(0,io->memory.input.data(),Fixture::InputBytes));
        assert(Fixture::FillOutputBytes(0,io->memory.output.data(),Fixture::OutputBytes));
        assert(service->Install(image,*io)==DataflowServiceComplete);io->route=0;
        assert(qualification->Bind(scratch,scope,0,BankBase,TestRoutes));
        if (scenario==1) scope.failRestore=37;
        if (scenario==2) io->failRead=37;
        const auto result=qualification->Run(*service,*io,scratch,scope);
        if (!scenario) {
            assert(result==DataflowServiceComplete && qualification->IsAudited() && !scope.retained
                && io->route==0 && io->projected==DataflowCoreCount && scope.restores>DataflowCoreCount);
            for (unsigned row=0;row<Fixture::Rows;row++) {
                uint32_t bits=0;std::memcpy(&bits,io->memory.output.data()+row*4,4);
                assert(Fixture::OutputMatches(row,bits));
            }
        } else {
            assert(result==DataflowServiceProjectionFailed && qualification->IsFailed()
                && scope.retained && io->route==37 && !io->projected);
        }
    }
    std::puts("PASS 120-worker routed projection and uncertain-route failure retention; SIMULATED ONLY");
}
